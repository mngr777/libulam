#include <algorithm>
#include <libulam/assert.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/utils/integer.hpp>

namespace ulam {

TypedValue IntType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf: {
        auto int_val = utils::integer_min(bitsize());
        return {this, Value::make_r(int_val, value::IsConsteval)};
    }
    case TypeOp::MaxOf: {
        auto int_val = utils::integer_max(bitsize());
        return {this, Value::make_r(int_val, value::IsConsteval)};
    }
    default:
        return _PrimType::type_op(op);
    }
}

RValue IntType::construct_default(value::flags_t rval_flags) {
    return RValue::make(Integer{}, rval_flags);
}

RValue IntType::construct(Integer int_val, value::flags_t rval_flags) {
    int_val = std::min(int_val, utils::integer_max(bitsize()));
    return RValue::make(int_val, rval_flags);
}

RValue IntType::from_datum(Datum datum) {
    Integer int_val = utils::integer_from_datum(datum, bitsize());
    return RValue::make(int_val);
}

Datum IntType::to_datum(const RValue& rval) {
    ulam_assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    return utils::integer_to_datum(int_val, bitsize());
}

TypedValue IntType::unary_op(Op op, RValue&& rval) {
    if (!rval.has_rvalue())
        return {this, Value{std::move(rval)}};

    auto int_val = rval.get<Integer>();
    value::flags_t rval_flags = value::IsConsteval * rval.is_consteval();

    switch (op) {
    case Op::UnaryMinus:
        ulam_assert(int_val >= utils::integer_min(bitsize()));
        int_val = (int_val == utils::integer_min(bitsize()))
                      ? utils::integer_max(bitsize())
                      : -int_val;
        break;
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        rval_flags &= ~value::IsConsteval;
        if (int_val < utils::integer_max(bitsize()))
            ++int_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        rval_flags &= ~value::IsConsteval;
        if (int_val > utils::integer_min(bitsize()))
            --int_val;
        break;
    default:
        unreachable();
    }
    return {this, Value::make_r(int_val, rval_flags)};
}

TypedValue IntType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    ulam_assert(r_type->is(IntId));
    ulam_assert(!l_rval.has_rvalue() || l_rval.is<Integer>());
    ulam_assert(!r_rval.has_rvalue() || r_rval.is<Integer>());

    bool is_unknown = !l_rval.has_rvalue() || !r_rval.has_rvalue();
    Integer l_int = l_rval.has_rvalue() ? l_rval.get<Integer>() : 0;
    Integer r_int = r_rval.has_rvalue() ? r_rval.get<Integer>() : 0;

    bool is_wider =
        (bitsize() > DefaultSize || r_type->bitsize() > DefaultSize);
    bitsize_t max_size = is_wider ? MaxSize : DefaultSize;

    bool is_consteval = !ops::is_assign(op) && !is_unknown &&
                        l_rval.is_consteval() && r_rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int == r_int, rval_flags)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int != r_int, rval_flags)}};
    }
    case Op::AssignProd: {
        // (Int(a) += Int(b)) == Int(a)
        if (is_unknown)
            return {this, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_prod(l_int, r_int);
        int_val = utils::truncate(int_val, bitsize());
        return {this, Value::make_r(int_val, rval_flags)};
    }
    case Op::Prod: {
        // Int(a) * Int(b) = Int(a + b)
        auto size =
            std::min<bitsize_t>(max_size, bitsize() + r_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_prod(l_int, r_int);
        int_val = utils::truncate(int_val, size);
        return {type, Value::make_r(int_val, rval_flags)};
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Int(a) / Int(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, Value::make_r_ph()};
        auto int_val = utils::safe_quot(l_int, r_int);
        return {this, Value::make_r(int_val, rval_flags)};
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Int(a) % Int(b) = Int(a)
        if (is_unknown)
            return {this, Value::make_r_ph()};
        auto int_val = utils::safe_rem(l_int, r_int);
        return {this, Value::make_r(int_val, rval_flags)};
    }
    case Op::AssignSum: {
        // (Int(a) += Int(b)) = Int(a)
        if (is_unknown)
            return {this, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_sum(l_int, r_int);
        int_val = utils::truncate(int_val, bitsize());
        return {this, Value::make_r(int_val, rval_flags)};
    }
    case Op::Sum: {
        // Int(a) + Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_sum(l_int, r_int);
        return {type, Value::make_r(int_val, rval_flags)};
    }
    case Op::AssignDiff: {
        // (Int(a) -= Int(b)) = Int(a)
        if (is_unknown)
            return {this, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_diff(l_int, r_int);
        int_val = utils::truncate(int_val, bitsize());
        return {this, Value::make_r(int_val, rval_flags)};
    }
    case Op::Diff: {
        // Int(a) - Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value::make_r_ph()};
        auto [int_val, _] = utils::safe_diff(l_int, r_int);
        return {type, Value::make_r(int_val, rval_flags)};
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int < r_int, rval_flags)}};
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int <= r_int, rval_flags)}};
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int > r_int, rval_flags)}};
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_int >= r_int, rval_flags)}};
    }
    default:
        unreachable();
    }
}

bool IntType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case IntId:
        return expl || type->bitsize() >= bitsize();
    case UnsignedId:
        return expl;
    case BoolId:
        return false;
    case UnaryId:
        return expl;
    case BitsId:
        return expl || type->bitsize() >= bitsize();
    case AtomId:
        return false;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        unreachable();
    }
}

bool IntType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return expl;
    case BoolId:
        return false;
    case UnaryId:
        return expl;
    case BitsId:
        return true;
    case AtomId:
        return false;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        unreachable();
    }
}

bool IntType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    auto rval = val.copy_rvalue();
    if (!rval.has_rvalue() || !rval.is_consteval())
        return is_castable_to_prim(type, false);

    ulam_assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();

    switch (type->bi_type_id()) {
    case IntId:
        return utils::bitsize(int_val) <= type->bitsize();
    case UnsignedId:
        return int_val >= 0 &&
               utils::bitsize((Unsigned)int_val) <= type->bitsize();
    case BitsId:
        return utils::bitsize(int_val) <= type->bitsize();
    case UnaryId:
        return int_val >= 0 && (Unsigned)int_val <= type->bitsize();
    default:
        return is_castable_to_prim(type, false);
    }
}

bool IntType::is_impl_castable_to_prim(
    BuiltinTypeId bi_type_id, const Value& val) const {
    if (bi_type_id != UnsignedId && bi_type_id != UnaryId)
        return is_castable_to_prim(bi_type_id, false);

    auto rval = val.copy_rvalue();
    if (!rval.has_rvalue() || !rval.is_consteval())
        return is_castable_to_prim(bi_type_id, false);

    ulam_assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    return int_val >= 0;
}

TypedValue IntType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    ulam_assert(is_expl_castable_to(id));

    bool is_unknown = !rval.has_rvalue();
    bool is_wider = bitsize() > DefaultSize;
    bool is_consteval = !is_unknown && rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;
    auto int_val = is_unknown ? 0 : rval.get<Integer>();

    switch (id) {
    case IntId: {
        unreachable();
        // return {this, Value{std::move(rval)}};
    }
    case UnsignedId: {
        Unsigned uns_val = std::max<Integer>(0, int_val);
        if (is_consteval) {
            auto size = utils::bitsize(uns_val);
            auto type = builtins().unsigned_type(size);
            return {type, Value{type->construct(uns_val, rval_flags)}};
        } else {
            auto size = std::max<bitsize_t>(1, bitsize() - 1);
            auto type = builtins().unsigned_type(size);
            if (is_unknown)
                return {type, Value::make_r_ph()};
            return {type, Value{type->construct(uns_val, rval_flags)}};
        }
    }
    case UnaryId: {
        bitsize_t max_size =
            is_wider ? UnaryType::MaxSize : UnaryType::DefaultSize;
        Unsigned uns_val = std::max<Integer>(0, int_val);
        if (is_consteval) {
            uns_val = std::min<Unsigned>(uns_val, max_size);
            auto type = builtins().unary_type(uns_val);
            return {
                type, Value{type->construct(utils::ones(uns_val), rval_flags)}};
        } else {
            auto size = max_size;
            if (bitsize() - 1 < max_size) {
                size = (1 << (bitsize() - 1)) - 1;
                size = std::min<bitsize_t>(size, max_size);
            }
            uns_val = std::min<Unsigned>(uns_val, size);
            auto type = builtins().unary_type(size);
            if (is_unknown)
                return {type, Value::make_r_ph()};
            return {type, Value{type->construct(uns_val, rval_flags)}};
        }
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        if (is_unknown)
            return {type, Value::make_r_ph()};
        Bits bits_val{size};
        store(bits_val.view(), 0, rval);
        return {type, Value::make_r(std::move(bits_val), rval_flags)};
    }
    default:
        unreachable();
    }
}

RValue IntType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    ulam_assert(is_expl_castable_to(type));
    if (!rval.has_rvalue())
        return std::move(rval);

    auto int_val = rval.get<Integer>();
    value::flags_t rval_flags = value::IsConsteval * rval.is_consteval();

    switch (type->bi_type_id()) {
    case IntId: {
        auto int_min = utils::integer_min(type->bitsize());
        if (int_val < int_min)
            return RValue::make(int_min, rval_flags);
        auto int_max = utils::integer_max(type->bitsize());
        if (int_val > int_max)
            return RValue::make(int_max, rval_flags);
        return std::move(rval);
    }
    case UnsignedId: {
        Unsigned uns_val = std::max((Integer)0, int_val);
        uns_val = std::min(utils::unsigned_max(type->bitsize()), uns_val);
        return RValue::make(uns_val, rval_flags);
    }
    case UnaryId: {
        Unsigned uns_val = std::max((Integer)0, int_val);
        uns_val = std::min(uns_val, (Unsigned)type->bitsize());
        return RValue::make(utils::ones(uns_val), rval_flags);
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        int_val = std::min(int_val, utils::integer_max(size));
        int_val = std::max(int_val, utils::integer_min(size));
        auto datum = utils::integer_to_datum(int_val, size);
        auto bits_rval = type->construct_default(rval_flags);
        bits_rval.get<Bits>().write_right(size, datum);
        return bits_rval;
    }
    default:
        unreachable();
    }
}

} // namespace ulam

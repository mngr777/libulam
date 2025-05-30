#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/detail/integer.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

TypedValue IntType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{detail::integer_min(bitsize()), true}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{detail::integer_max(bitsize()), true}}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue IntType::construct() { return RValue{Integer{}}; }

RValue IntType::construct(Integer int_val) { return RValue{int_val}; }

RValue IntType::from_datum(Datum datum) {
    Integer int_val = detail::integer_from_datum(datum, bitsize());
    return RValue{int_val};
}

Datum IntType::to_datum(const RValue& rval) {
    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    return detail::integer_to_datum(int_val, bitsize());
}

TypedValue IntType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{std::move(rval)}};

    bool is_consteval = rval.is_consteval();
    auto int_val = rval.get<Integer>();
    switch (op) {
    case Op::UnaryMinus:
        assert(int_val >= detail::integer_min(bitsize()));
        int_val = (int_val == detail::integer_min(bitsize()))
                      ? detail::integer_max(bitsize())
                      : -int_val;
        break;
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        is_consteval = false;
        if (int_val < detail::integer_max(bitsize()))
            ++int_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        is_consteval = false;
        if (int_val > detail::integer_min(bitsize()))
            --int_val;
        break;
    default:
        assert(false);
    }
    return {this, Value{RValue{int_val, is_consteval}}};
}

TypedValue IntType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(IntId));
    assert(l_rval.empty() || l_rval.is<Integer>());
    assert(r_rval.empty() || r_rval.is<Integer>());

    bool is_unknown = l_rval.empty() || r_rval.empty();
    Integer l_int = l_rval.empty() ? 0 : l_rval.get<Integer>();
    Integer r_int = r_rval.empty() ? 0 : r_rval.get<Integer>();

    bool is_wider =
        (bitsize() > DefaultSize || r_type->bitsize() > DefaultSize);
    bitsize_t max_size = is_wider ? MaxSize : DefaultSize;

    bool is_consteval = !ops::is_assign(op) && !is_unknown &&
                        l_rval.is_consteval() && r_rval.is_consteval();

    auto make_res = [&](Ref<Type> type, RValue&& rval) -> TypedValue {
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    };
    auto make_empty = [&](Ref<Type> type) -> TypedValue {
        return {type, Value{RValue{}}};
    };

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        return make_res(type, type->construct(l_int == r_int));
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        return make_res(type, type->construct(l_int != r_int));
    }
    case Op::AssignProd: {
        // (Int(a) += Int(b)) == Int(a)
        auto [val, _] = detail::safe_prod(l_int, r_int);
        return make_res(this, RValue{detail::truncate(val, bitsize())});
    }
    case Op::Prod: {
        // Int(a) * Int(b) = Int(a + b)
        auto size =
            std::min<bitsize_t>(max_size, bitsize() + r_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = detail::safe_prod(l_int, r_int);
        return make_res(type, RValue{detail::truncate(val, size)});
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Int(a) / Int(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return make_empty(this);
        return make_res(this, RValue{detail::safe_quot(l_int, r_int)});
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Int(a) % Int(b) = Int(a)
        if (is_unknown)
            return make_empty(this);
        return make_res(this, RValue{detail::safe_rem(l_int, r_int)});
    }
    case Op::AssignSum: {
        // (Int(a) += Int(b)) = Int(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = detail::safe_sum(l_int, r_int);
        return make_res(this, RValue{detail::truncate(val, bitsize())});
    }
    case Op::Sum: {
        // Int(a) + Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = detail::safe_sum(l_int, r_int);
        return make_res(type, RValue{val});
    }
    case Op::AssignDiff: {
        // (Int(a) -= Int(b)) = Int(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = detail::safe_diff(l_int, r_int);
        return make_res(this, RValue{detail::truncate(val, bitsize())});
    }
    case Op::Diff: {
        // Int(a) - Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = detail::safe_diff(l_int, r_int);
        return make_res(type, RValue{val});
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_int < r_int));
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_int <= r_int));
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_int > r_int));
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_int >= r_int));
    }
    default:
        assert(false);
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
        assert(false);
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
        assert(false);
    }
}

bool IntType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    auto rval = val.copy_rvalue();
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(type, false);

    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();

    switch (type->bi_type_id()) {
    case IntId:
        return detail::bitsize(int_val) <= type->bitsize();
    case UnsignedId:
        return int_val >= 0 &&
               detail::bitsize((Unsigned)int_val) <= type->bitsize();
    case BitsId:
        return detail::bitsize(int_val) <= type->bitsize();
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
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(bi_type_id, false);

    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    return int_val >= 0;
}

TypedValue IntType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));

    bool is_unknown = rval.empty();
    bool is_wider = bitsize() > DefaultSize;
    bool is_consteval = !is_unknown && rval.is_consteval();
    auto int_val = is_unknown ? 0 : rval.get<Integer>();

    switch (id) {
    case IntId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case UnsignedId: {
        Unsigned uns_val = std::max<Integer>(0, int_val);
        if (is_consteval) {
            auto size = detail::bitsize(uns_val);
            auto type = builtins().unsigned_type(size);
            return {type, Value{RValue{uns_val, true}}};
        } else {
            auto size = std::max<bitsize_t>(1, bitsize() - 1);
            auto type = builtins().unsigned_type(size);
            Value val{!is_unknown ? RValue{uns_val} : RValue{}};
            return {type, std::move(val)};
        }
    }
    case UnaryId: {
        bitsize_t max_size =
            is_wider ? UnaryType::MaxSize : UnaryType::DefaultSize;
        Unsigned uns_val = std::max<Integer>(0, int_val);
        if (is_consteval) {
            uns_val = std::min<Unsigned>(uns_val, max_size);
            auto type = builtins().unary_type(uns_val);
            return {type, Value{RValue{detail::ones(uns_val), true}}};
        } else {
            auto size = max_size;
            if (bitsize() - 1 < max_size) {
                size = (1 << (bitsize() - 1)) - 1;
                size = std::min<bitsize_t>(size, max_size);
            }
            uns_val = std::min<Unsigned>(uns_val, size);
            auto type = builtins().unary_type(size);
            Value val{!is_unknown ? RValue{detail::ones(uns_val)} : RValue{}};
            return {type, std::move(val)};
        }
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        Bits val{size};
        store(val.view(), 0, rval);
        return {type, Value{RValue{std::move(val), is_consteval}}};
    }
    default:
        assert(false);
    }
}

RValue IntType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    if (rval.empty())
        return std::move(rval);

    auto int_val = rval.get<Integer>();
    bool is_consteval = rval.is_consteval();
    switch (type->bi_type_id()) {
    case IntId: {
        auto int_min = detail::integer_min(type->bitsize());
        if (int_val < int_min)
            return RValue{int_min};
        auto int_max = detail::integer_max(type->bitsize());
        if (int_val > int_max)
            return RValue{int_max, is_consteval};
        return std::move(rval);
    }
    case UnsignedId: {
        Unsigned val = std::max((Integer)0, int_val);
        val = std::min(detail::unsigned_max(type->bitsize()), val);
        return RValue{val, is_consteval};
    }
    case UnaryId: {
        Unsigned val = std::max((Integer)0, int_val);
        val = std::min(val, (Unsigned)type->bitsize());
        return RValue{detail::ones(val), is_consteval};
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        int_val = std::min(int_val, detail::integer_max(size));
        int_val = std::max(int_val, detail::integer_min(size));
        auto datum = detail::integer_to_datum(int_val, size);
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(size, datum);
        bits_rval.set_is_consteval(is_consteval);
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam

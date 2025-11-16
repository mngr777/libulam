#include <cassert>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/utils/integer.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

TypedValue UnsignedType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{(Unsigned)0, true}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{utils::unsigned_max(bitsize()), true}}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue UnsignedType::construct() { return RValue{Unsigned{}}; }

RValue UnsignedType::from_datum(Datum datum) { return RValue{(Unsigned)datum}; }

Datum UnsignedType::to_datum(const RValue& rval) {
    assert(rval.is<Unsigned>());
    auto uns_val = rval.get<Unsigned>();
    return (Datum)uns_val;
}

TypedValue UnsignedType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{RValue{}}};

    bool is_consteval = rval.is_consteval();
    auto uns_val = rval.get<Unsigned>();
    switch (op) {
    case Op::UnaryMinus: {
        bitsize_t max_size = (bitsize() > DefaultSize) ? ULAM_MAX_INT_SIZE
                                                       : IntType::DefaultSize;
        auto size = std::min<bitsize_t>(bitsize() + 1, max_size);
        auto int_val = std::max<Integer>(utils::integer_min(size), -uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{int_val, is_consteval}}};
    }
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        is_consteval = false;
        if (uns_val < utils::unsigned_max(bitsize()))
            ++uns_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        is_consteval = false;
        if (uns_val > 0)
            --uns_val;
        break;
    default:
        assert(false);
    }
    return {this, Value{RValue{uns_val, is_consteval}}};
}

TypedValue UnsignedType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(UnsignedId));
    assert(l_rval.empty() || l_rval.is<Unsigned>());
    assert(r_rval.empty() || r_rval.is<Unsigned>());

    bool is_unknown = l_rval.empty() || r_rval.empty();
    Unsigned l_uns = l_rval.empty() ? 0 : l_rval.get<Unsigned>();
    Unsigned r_uns = r_rval.empty() ? 0 : r_rval.get<Unsigned>();

    bool is_wider = bitsize() > DefaultSize || r_type->bitsize() > DefaultSize;
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
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns == r_uns));
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns != r_uns));
    }
    case Op::AssignProd: {
        // (Unsigned(a) *= Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = utils::safe_prod(l_uns, r_uns);
        return make_res(this, RValue{utils::truncate(val, bitsize())});
    }
    case Op::Prod: {
        // Unsigned(a) * Unsigned(b) = Unsigned(a + b)
        auto size =
            std::min<bitsize_t>(max_size, bitsize() + r_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = utils::safe_prod(l_uns, r_uns);
        return make_res(type, RValue{val});
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Unsigned(a) / Unsigned(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return make_empty(this);
        auto val = utils::safe_quot(l_uns, r_uns);
        return make_res(this, RValue{val});
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Unsigned(a) % Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        return make_res(this, RValue{utils::safe_rem(l_uns, r_uns)});
    }
    case Op::AssignSum: {
        // (Unsigned(a) += Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = utils::safe_sum(l_uns, r_uns);
        return make_res(this, RValue{utils::truncate(val, bitsize())});
    }
    case Op::Sum: {
        // Unsigned(a) + Unsigned(b) = Unsigned(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = utils::safe_sum(l_uns, r_uns);
        return make_res(type, RValue{val});
    }
    case Op::AssignDiff:
    case Op::Diff: {
        // Unsigned(a) - Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        Unsigned val = (l_uns > r_uns) ? l_uns - r_uns : 0;
        return make_res(this, RValue{val});
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns < r_uns));
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns <= r_uns));
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns > r_uns));
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns >= r_uns));
    }
    default:
        assert(false);
    }
}

bool UnsignedType::is_castable_to_prim(
    Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case IntId: {
        bool is_wider =
            (bitsize() > DefaultSize || type->bitsize() > IntType::DefaultSize);
        bitsize_t max_size = is_wider ? IntType::MaxSize : IntType::DefaultSize;
        return expl || type->bitsize() == max_size ||
               type->bitsize() > bitsize();
    }
    case UnsignedId:
        return expl || type->bitsize() >= bitsize();
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return expl || utils::unsigned_max(bitsize()) <= type->bitsize();
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

bool UnsignedType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return true;
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return true;
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

bool UnsignedType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    auto rval = val.copy_rvalue();
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(type, false);

    assert(rval.is<Unsigned>());
    auto uns_val = rval.get<Unsigned>();

    switch (type->bi_type_id()) {
    case IntId:
        if (uns_val == 0)
            return true;
        return utils::bitsize(uns_val) <= utils::integer_max(type->bitsize());
    case UnsignedId:
        if (uns_val == 0)
            return true;
        return utils::bitsize(uns_val) <= type->bitsize();
    case UnaryId:
        return uns_val <= type->bitsize();
    default:
        return is_castable_to_prim(type, false);
    }
}

TypedValue UnsignedType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));

    bool is_unknown = rval.empty();
    bool is_wider = bitsize() > DefaultSize;
    bool is_consteval = !is_unknown && rval.is_consteval();
    auto uns_val = is_unknown ? 0 : rval.get<Unsigned>();

    switch (id) {
    case IntId: {
        bitsize_t max_size = is_wider ? IntType::MaxSize : IntType::DefaultSize;
        if (is_consteval) {
            auto size =
                std::min<bitsize_t>(utils::bitsize(uns_val) + 1, max_size);
            auto type = builtins().int_type(size);
            Integer int_val =
                std::min((Unsigned)utils::integer_max(size), uns_val);
            return {type, Value{RValue{int_val, true}}};
        } else {
            auto size = std::min<bitsize_t>(bitsize() + 1, max_size);
            auto type = builtins().int_type(size);
            Integer int_val =
                std::min((Unsigned)utils::integer_max(size), uns_val);
            return {type, Value{!is_unknown ? RValue{int_val} : RValue{}}};
        }
    }
    case UnsignedId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto boolean = builtins().boolean();
        if (is_unknown)
            return {boolean, Value{RValue{}}};
        auto rval = boolean->construct(uns_val > 0);
        rval.set_is_consteval(is_consteval);
        return {boolean, Value{std::move(rval)}};
    }
    case UnaryId: {
        bitsize_t max_size =
            is_wider ? UnaryType::MaxSize : UnaryType::DefaultSize;
        if (is_consteval) {
            auto size = std::min<bitsize_t>(max_size, uns_val);
            auto type = builtins().unary_type(size);
            uns_val = std::min<Unsigned>(uns_val, size);
            return {type, Value{RValue{uns_val, true}}};
        } else {
            auto size = max_size;
            if (bitsize() < max_size) {
                size = (1 << bitsize()) - 1;
                size = std::min<bitsize_t>(size, max_size);
            }
            uns_val = std::min<Unsigned>(uns_val, size);
            auto type = builtins().unary_type(size);
            Value val{!is_unknown ? RValue{utils::ones(uns_val)} : RValue{}};
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

RValue UnsignedType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    if (rval.empty())
        return std::move(rval);

    Unsigned uns_val = rval.get<Unsigned>();
    bool is_consteval = rval.is_consteval();
    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = utils::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val, is_consteval};
    }
    case UnsignedId: {
        auto uns_max = utils::unsigned_max(type->bitsize());
        if (uns_val > uns_max)
            return RValue{uns_max, is_consteval};
        return std::move(rval);
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto rval =
            builtins().bool_type(type->bitsize())->construct(uns_val > 0);
        rval.set_is_consteval(is_consteval);
        return rval;
    }
    case UnaryId: {
        uns_val = std::min<Unsigned>(uns_val, type->bitsize());
        Unsigned val = utils::ones(uns_val);
        return RValue{val, is_consteval};
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(size, (Datum)uns_val);
        bits_rval.set_is_consteval(is_consteval);
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam

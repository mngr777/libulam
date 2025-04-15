#include <libulam/semantic/detail/integer.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

Unsigned UnaryType::unsigned_value(const RValue& rval) {
    assert(!rval.empty());
    Unsigned uns_val = detail::count_ones(rval.get<Unsigned>());
    assert(uns_val <= bitsize());
    return uns_val;
}

TypedValue UnaryType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf: {
        auto rval = construct(0);
        rval.set_is_consteval(true);
        return {this, Value{std::move(rval)}};
    }
    case TypeOp::MaxOf: {
        auto rval = construct(bitsize());
        rval.set_is_consteval(true);
        return {this, Value{std::move(rval)}};
    }
    default:
        return _PrimType::type_op(op);
    }
}

RValue UnaryType::construct() { return construct(0); }

RValue UnaryType::construct(Unsigned uns_val) {
    assert(uns_val <= bitsize());
    return RValue{detail::ones(uns_val)};
}

RValue UnaryType::from_datum(Datum datum) { return RValue{(Unsigned)datum}; }

Datum UnaryType::to_datum(const RValue& rval) {
    assert(rval.is<Unsigned>());
    return (Datum)rval.get<Unsigned>();
}

TypedValue UnaryType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{RValue{}}};

    bool is_consteval = rval.is_consteval();
    Unsigned uns_val = unsigned_value(rval);
    switch (op) {
    case Op::UnaryMinus: {
        bitsize_t size = detail::bitsize(uns_val);
        assert(size <= IntType::DefaultSize);
        return {
            builtins().int_type(size),
            Value{RValue{(-(Integer)uns_val), is_consteval}}};
    }
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        is_consteval = false;
        if (uns_val < bitsize())
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
    return {this, Value{RValue(detail::ones(uns_val), is_consteval)}};
}

TypedValue UnaryType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(UnaryId));
    assert(l_rval.empty() || l_rval.is<Unsigned>());
    assert(r_rval.empty() || r_rval.is<Unsigned>());

    bool is_unknown = l_rval.empty() || r_rval.empty();

    Unsigned l_uns = l_rval.empty() ? 0 : unsigned_value(l_rval);
    auto r_unary_type = builtins().unary_type(r_type->bitsize());
    Unsigned r_uns = r_rval.empty() ? 0 : r_unary_type->unsigned_value(r_rval);

    bool is_consteval =
        !is_unknown && l_rval.is_consteval() && r_rval.is_consteval();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto rval = type->construct(l_uns == r_uns);
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto rval = type->construct(l_uns != r_uns);
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    }
    default:
        assert(false);
    }
}

bool UnaryType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->bi_type_id()) {
    case IntId:
        return expl || detail::integer_max(size) >= bitsize();
    case UnsignedId:
        return expl || detail::unsigned_max(size) >= bitsize();
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return expl || size >= bitsize();
    case BitsId:
        return expl || size >= bitsize();
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

bool UnaryType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
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

TypedValue UnaryType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));

    bool is_unknown = rval.empty();
    bool is_consteval = rval.is_consteval();
    auto uns_val = is_unknown ? 0 : unsigned_value(rval);

    switch (id) {
    case IntId: {
        Integer int_val = uns_val;
        if (is_consteval) {
            auto size = detail::bitsize(int_val);
            auto type = builtins().int_type(size);
            return {type, Value{RValue{int_val, true}}};
        } else {
            auto size = detail::bitsize((Integer)bitsize());
            auto type = builtins().int_type(size);
            return {type, Value{!is_unknown ? RValue{int_val} : RValue{}}};
        }
    }
    case UnsignedId: {
        if (is_consteval) {
            auto size = detail::bitsize(uns_val);
            auto type = builtins().unsigned_type(size);
            return {type, Value{RValue{uns_val, true}}};
        } else {
            auto size = detail::bitsize((Unsigned)bitsize());
            auto type = builtins().unsigned_type(size);
            return {type, Value{!is_unknown ? RValue{uns_val} : RValue{}}};
        }
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
        assert(false);
        return {this, Value{std::move(rval)}};
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

RValue UnaryType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    if (rval.empty())
        return std::move(rval);

    auto unary_type = builtins().unary_type(type->bitsize());
    Unsigned uns_val = unary_type->unsigned_value(rval);
    bool is_consteval = rval.is_consteval();
    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val, is_consteval};
    }
    case UnsignedId: {
        uns_val = std::min(detail::unsigned_max(type->bitsize()), uns_val);
        return RValue{uns_val, is_consteval};
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto rval =
            builtins().bool_type(type->bitsize())->construct(uns_val > 0);
        rval.set_is_consteval(is_consteval);
        return rval;
    }
    case UnaryId: {
        uns_val = std::min<Unsigned>(type->bitsize(), uns_val);
        return RValue{detail::ones(uns_val), is_consteval};
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        uns_val = std::min<bitsize_t>(uns_val, size);
        Datum datum = detail::ones(uns_val);
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

#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

TypedValue UnaryType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{(Unsigned)0, true}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{(Unsigned)bitsize(), true}}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue UnaryType::construct() const { return RValue{Unsigned{}}; }

RValue UnaryType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnaryType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

TypedValue UnaryType::unary_op(Op op, RValue&& rval) {
    assert(false); // not implemented
}

TypedValue UnaryType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(UnaryId));
    assert(l_rval.empty() || l_rval.is<Unsigned>());
    assert(r_rval.empty() || r_rval.is<Unsigned>());

    bool is_unknown = l_rval.empty() || r_rval.empty();

    Unsigned l_uns =
        l_rval.empty() ? 0 : detail::count_ones(l_rval.get<Unsigned>());
    Unsigned r_uns =
        r_rval.empty() ? 0 : detail::count_ones(r_rval.get<Unsigned>());

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
    assert(!rval.empty());
    assert(rval.is<Unsigned>());

    auto uns_val = detail::count_ones(rval.get<Unsigned>());
    switch (id) {
    case IntId: {
        auto size = std::min(
            (bitsize_t)ULAM_MAX_INT_SIZE,
            (bitsize_t)(detail::bitsize(uns_val) + 1));
        Integer val = std::min((Unsigned)detail::integer_max(size), uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{val}}};
    }
    case UnsignedId: {
        auto size = detail::bitsize(uns_val);
        auto type = builtins().prim_type(UnsignedId, size);
        return {type, Value{RValue{uns_val}}};
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto boolean = builtins().boolean();
        return {boolean, Value{boolean->construct(uns_val > 0)}};
    }
    case UnaryId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        store(val.view(), 0, rval);
        return {type, Value{RValue{std::move(val)}}};
    }
    default:
        assert(false);
    }
}

RValue UnaryType::cast_to_prim(Ref<const PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = detail::count_ones(rval.get<Unsigned>());
    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        uns_val = std::min(detail::unsigned_max(type->bitsize()), uns_val);
        return RValue{uns_val};
    }
    case BoolId: {
        assert(bitsize() == 1);
        return builtins().bool_type(type->bitsize())->construct(uns_val > 0);
    }
    case UnaryId: {
        uns_val = std::min<Unsigned>(type->bitsize(), uns_val);
        return RValue{detail::ones(uns_val)};
    }
    case BitsId: {
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(type->bitsize(), to_datum(rval));
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam

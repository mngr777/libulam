#include "libulam/semantic/value/types.hpp"
#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue UnsignedType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnsignedType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

bool UnsignedType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return true;
    case BoolId:
        return false;
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

bool UnsignedType::is_castable_to(Ref<const PrimType> type, bool expl) const {
    switch (type->builtin_type_id()) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl || type->bitsize() >= bitsize();
    case BoolId:
        return false;
    case UnaryId:
        return expl;
    case BitsId:
        return expl;
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

PrimTypedValue UnsignedType::cast_to(BuiltinTypeId id, Value&& value) {
    assert(is_expl_castable_to(id));
    assert(!value.empty());

    auto rval = value.move_rvalue();
    assert(rval.is<Unsigned>());

    auto uns_val = rval.get<Unsigned>();
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
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)ULAM_MAX_INT_SIZE, uns_val);
        auto type = builtins().prim_type(UnaryId, detail::ones(uns_val));
        return {type, Value{RValue{val}}};
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        store(val.bits().view(), 0, rval);
        return {type, Value{RValue{std::move(val)}}};
    }
    default:
        assert(false);
    }
}

RValue UnsignedType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = rval.get<Unsigned>();
    switch (type->builtin_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        auto uns_max = detail::unsigned_max(type->bitsize());
        if (uns_val > uns_max)
            return RValue{uns_max};
        return std::move(rval);
    }
    case UnaryId: {
        Unsigned val =
            std::min((Unsigned)type->bitsize(), detail::ones(uns_val));
        return RValue{val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue UnsignedType::unary_op(Op op, RValue&& rval) {
    assert(false); // not implemented
}

PrimTypedValue UnsignedType::binary_op(
    Op op,
    Value&& left_val,
    Ref<const PrimType> right_type,
    Value&& right_val) {
    assert(right_type->is(UnsignedId));
    assert(false); // not implemented
}

} // namespace ulam

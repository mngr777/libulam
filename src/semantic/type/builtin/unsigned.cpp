#include "libulam/semantic/value/types.hpp"
#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue UnsignedType::from_datum(Datum datum) const { return (Unsigned)datum; }

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
        return true;
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

bool UnsignedType::is_castable_to(Ref<PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->builtin_type_id()) {
    case IntId:
        return expl || size + 1 <= bitsize();
    case UnsignedId:
        return expl || size <= bitsize();
    case BoolId:
        return true;
    case UnaryId:
        return expl || detail::unary_unsigned_bitsize(size) <= bitsize();
    case BitsId:
        return expl || size <= bitsize();
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

    auto rval = value.rvalue();
    assert(rval.is<Unsigned>());

    auto uns_val = rval.get<Unsigned>();
    switch (id) {
    case IntId: {
        auto size = std::min(
            (bitsize_t)ULAM_MAX_INT_SIZE,
            (bitsize_t)(detail::bitsize(uns_val) + 1));
        Integer val = std::min((Unsigned)detail::integer_max(size), uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, RValue{val}};
    }
    case UnsignedId: {
        assert(false);
        return {this, std::move(value)};
    }
    case BoolId: {
        Unsigned val = (uns_val == 0) ? 0 : 1;
        auto type = builtins().prim_type(BoolId, 1);
        return {type, RValue{val}};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)ULAM_MAX_INT_SIZE, uns_val);
        auto type = builtins().prim_type(UnaryId, uns_val);
        return {type, RValue{val}};
    }
    case BitsId: {
        auto size = detail::bitsize(uns_val);
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        store(val.bits(), 0, rval);
        return {type, RValue{std::move(val)}};
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
        Integer val =
            std::min(detail::integer_max(type->bitsize()), (Integer)uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        auto uns_max = detail::unsigned_max(type->bitsize());
        if (uns_val > uns_max)
            return RValue{uns_max};
        return std::move(rval);
    }
    case BoolId: {
        Unsigned val = (uns_val == 0) ? (Unsigned)0
                                      : detail::unsigned_max(type->bitsize());
        return RValue{val};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)type->bitsize(), uns_val);
        return RValue{val};
    }
    default:
        assert(false);
    }
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

#include "libulam/semantic/value/types.hpp"
#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

bool UnsignedType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return true;
    case UnaryId:
        return true;
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
    assert(!value.is_nil());
    assert(value.rvalue()->is<Unsigned>());
    assert(!value.rvalue()->empty());

    auto rval = value.rvalue();
    auto unsval = rval->get<Unsigned>();
    switch (id) {
    case IntId: {
        auto size = std::min(
            (bitsize_t)ULAM_MAX_INT_SIZE,
            (bitsize_t)(detail::bitsize(value) + 1));
        Integer val = std::min((Unsigned)detail::integer_max(size), unsval);
        auto type = builtins().prim_type(IntId, size);
        return {type, RValue{val}};
    }
    case UnsignedId: {
        assert(false);
        return {this, std::move(value)};
    }
    case BoolId: {
        Unsigned val = (unsval == 0) ? 0 : 1;
        auto type = builtins().prim_type(BoolId, 1);
        return {type, RValue{val}};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)ULAM_MAX_INT_SIZE, unsval);
        auto type = builtins().prim_type(UnaryId, value);
        return {type, RValue{val}};
    }
    case BitsId: {
        auto size = detail::bitsize(unsval);
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        // TODO: write `unsval` bits
        return {type, RValue{std::move(val)}};
    }
    default:
        assert(false);
    }
}

Value UnsignedType::cast_to(Ref<PrimType> type, Value&& value) {
    assert(is_expl_castable_to(type));
    assert(!value.is_nil());
    assert(value.rvalue()->is<Unsigned>());
    assert(!value.rvalue()->empty());

    auto rval = value.rvalue();
    Unsigned unsval = rval->get<Unsigned>();
    switch (type->builtin_type_id()) {
    case IntId: {
        Integer val =
            std::min(detail::integer_max(type->bitsize()), (Integer)unsval);
        return RValue{val};
    }
    case UnsignedId: {
        assert(false);
        return std::move(value);
    }
    case BoolId: {
        Unsigned val =
            (unsval == 0) ? (Unsigned)0 : detail::unsigned_max(type->bitsize());
        return RValue{val};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)type->bitsize(), unsval);
        return RValue{val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue UnsignedType::binary_op(
    Op op,
    const Value& left_val,
    Ref<const PrimType> right_type,
    const Value& right_val) {
    assert(right_type->is(UnsignedId));
    assert(false); // not implemented
}

} // namespace ulam

#include "libulam/semantic/type/builtin/unsigned.hpp"
#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue UnaryType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnaryType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

bool UnaryType::is_castable_to(BuiltinTypeId id, bool expl) const {
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

bool UnaryType::is_castable_to(Ref<const PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->builtin_type_id()) {
    case IntId:
        return expl || detail::integer_max(size) >= bitsize();
    case UnsignedId:
        return expl || detail::unsigned_max(size) >= bitsize();
    case BoolId:
        return false;
    case UnaryId:
        return expl || size >= bitsize();
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

PrimTypedValue UnaryType::cast_to(BuiltinTypeId id, Value&& value) {
    assert(is_expl_castable_to(id));
    assert(!value.empty());

    auto rval = value.move_rvalue();
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
    case UnaryId: {
        assert(false);
        return {this, Value{std::move(rval)}};
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

RValue UnaryType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = detail::count_ones(rval.get<Unsigned>());
    switch (type->builtin_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        uns_val = std::min(detail::unsigned_max(type->bitsize()), uns_val);
        return RValue{uns_val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue UnaryType::unary_op(Op op, RValue&& rval) {
    assert(false); // not implemented
}

PrimTypedValue UnaryType::binary_op(
    Op op,
    Value&& left_val,
    Ref<const PrimType> right_type,
    Value&& right_val) {
    assert(right_type->is(UnsignedId));
    assert(false); // not implemented
}

} // namespace ulam

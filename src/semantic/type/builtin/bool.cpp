#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue BoolType::construct(bool value) {
    return RValue{detail::ones(bitsize())};
}

bool BoolType::is_true(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    auto uns_val = rval.get<Unsigned>();
    return detail::count_ones(uns_val) >= bitsize() + 1 / 2;
}

RValue BoolType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum BoolType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>(); // ??
}

bool BoolType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return true;
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

bool BoolType::is_castable_to(Ref<const PrimType> type, bool expl) const {
    switch (type->id()) {
    case BitsId:
        return type->bitsize() >= bitsize();
    default:
        return is_castable_to(type->builtin_type_id(), expl);
    }
}

PrimTypedValue BoolType::cast_to(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));
    switch (id) {
    case IntId: {
        auto type = builtins().prim_type(IntId, 2);
        return {type, Value{cast_to(type, std::move(rval))}};
    }
    case UnsignedId: {
        auto type = builtins().prim_type(UnsignedId, 1);
        return {type, Value{cast_to(type, std::move(rval))}};
    }
    case UnaryId: {
        auto type = builtins().prim_type(UnaryId, 1);
        return {type, Value{cast_to(type, std::move(rval))}};
    }
    case BoolId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case BitsId: {
        auto type = builtins().prim_type(BitsId, bitsize());
        return {type, Value{cast_to(type, std::move(rval))}};
    }
    default:
        assert(false);
    }
}

RValue BoolType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    bool is_truth = is_true(rval);
    rval = construct(is_truth);
    switch (type->builtin_type_id()) {
    case IntId: {
        return RValue{(Integer)(is_truth ? 1 : 0)};
    }
    case UnsignedId: {
        return RValue{(Unsigned)(is_truth ? 1 : 0)};
    }
    case UnaryId: {
        return RValue{(Unsigned)(is_truth ? 1 : 0)};
    }
    case BoolId: {
        return RValue{(Unsigned)(is_truth ? detail::ones(type->bitsize()) : 0)};
    }
    case BitsId: {
        auto bits_rval = type->construct();
        auto size = std::min(bitsize(), type->bitsize());
        bits_rval.get<Bits>().bits().write_right(size, to_datum(rval));
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam

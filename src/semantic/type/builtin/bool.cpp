#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam {

RValue BoolType::from_datum(Datum datum) const { return (Unsigned)datum; }

Datum BoolType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
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

bool BoolType::is_castable_to(Ref<PrimType> type, bool expl) const {
    return is_castable_to(type->builtin_type_id());
}

PrimTypedValue BoolType::cast_to(BuiltinTypeId id, Value&& value) {
    assert(is_expl_castable_to(id));
    assert(false);
}

RValue BoolType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    auto uns_val = rval.get<Unsigned>();
    bool is_truth = (detail::count_ones(uns_val) >= bitsize() / 2);
    switch (type->builtin_type_id()) {
    case IntId: {
        return RValue{(Integer)(is_truth ? 1 : 0)};
    }
    case UnsignedId: {
        return RValue{(Unsigned)(is_truth ? 1 : 0)};
    }
    case BoolId: {
        return RValue{(Unsigned)(is_truth ? detail::ones(type->bitsize()) : 0)};
    }
    default:
        assert(false);
    }
}

} // namespace ulam

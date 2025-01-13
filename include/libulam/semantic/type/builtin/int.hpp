#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Diag;
class Value;

class IntType : public _PrimType<IntId, 2, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(Builtins& builtins, TypeIdGen& id_gen, Ref<PrimTypeTpl> tpl, bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    bool is_castable_to(BuiltinTypeId id, bool expl = true) const override;
    bool is_castable_to(Ref<PrimType> type, bool expl = true) const override;

    PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) override;
    Value cast_to(Ref<PrimType> type, Value&& value) override;

    TypedValue binary_op(
        Op op,
        const Value& left_val,
        Ref<const PrimType> right_type,
        const Value& right_val) override;
};

using IntTypeTpl = _PrimTypeTpl<IntType>;

} // namespace ulam

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Diag;
class Value;

class IntType : public _PrimType<IntId, 2, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    TypedValue type_op(TypeOp op) override;

    RValue construct() override { return RValue{Integer{}}; }

    RValue from_datum(Datum datum) const override;
    Datum to_datum(const RValue& rval) const override;

    bool is_castable_to(BuiltinTypeId id, bool expl = true) const override;
    bool is_castable_to(Ref<const PrimType> type, bool expl = true) const override;

    PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) override;
    RValue cast_to(Ref<PrimType> type, RValue&& rval) override;

    PrimTypedValue binary_op(
        Op op,
        Value&& left_val,
        Ref<const PrimType> right_type,
        Value&& right_val) override;
};

using IntTypeTpl = _PrimTypeTpl<IntType>;

} // namespace ulam

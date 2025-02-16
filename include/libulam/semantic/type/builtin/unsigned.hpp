#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Value;

class UnsignedType : public _PrimType<UnsignedId, 1, ULAM_MAX_INT_SIZE, 32> {
public:
    UnsignedType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue construct() override { return RValue{Unsigned{}}; }

    RValue from_datum(Datum datum) const override;
    Datum to_datum(const RValue& rval) const override;

    bool is_castable_to(BuiltinTypeId id, bool expl = true) const override;
    bool is_castable_to(Ref<PrimType> type, bool expl = true) const override;

    PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) override;
    RValue cast_to(Ref<PrimType> type, RValue&& value) override;

    PrimTypedValue binary_op(
        Op op,
        Value&& left_val,
        Ref<const PrimType> right_type,
        Value&& right_val) override;
};

using UnsignedTypeTpl = _PrimTypeTpl<UnsignedType>;

} // namespace ulam

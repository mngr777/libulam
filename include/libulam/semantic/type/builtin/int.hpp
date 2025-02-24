#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class IntType : public _PrimType<IntId, 2, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    TypedValue type_op(TypeOp op) override;

    RValue construct() const override { return RValue{Integer{}}; }

    RValue from_datum(Datum datum) const override;
    Datum to_datum(const RValue& rval) const override;

    TypedValue cast_to(BuiltinTypeId id, RValue&& rval) override;
    RValue cast_to(Ref<const PrimType> type, RValue&& rval) override;

    TypedValue unary_op(Op op, RValue&& rval) override;

    TypedValue binary_op(
        Op op,
        RValue&& left_rval,
        Ref<const PrimType> right_type,
        RValue&& right_rval) override;

protected:
    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;

    bool is_castable_to_prim(
        BuiltinTypeId bi_type_id, bool expl = true) const override;

    bool is_impl_castable_to_prim(
        Ref<const PrimType> type, const Value& val) const override;

    bool is_impl_castable_to_prim(
        BuiltinTypeId bi_type_id, const Value& val) const override;
};

using IntTypeTpl = _PrimTypeTpl<IntType>;

} // namespace ulam

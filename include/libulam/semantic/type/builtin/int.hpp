#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

class IntType : public _PrimType<IntId, 1, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    TypedValue type_op(TypeOp op) override;

    RValue construct() override;
    RValue construct(Integer int_val);

    RValue from_datum(Datum datum) override;
    Datum to_datum(const RValue& rval) override;

    TypedValue unary_op(Op op, RValue&& rval) override;

    TypedValue binary_op(
        Op op,
        RValue&& l_rval,
        Ref<const PrimType> r_type,
        RValue&& r_rval) override;

protected:
    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;
    bool is_castable_to_prim(
        BuiltinTypeId bi_type_id, bool expl = true) const override;

    bool is_impl_castable_to_prim(
        Ref<const PrimType> type, const Value& val) const override;
    bool is_impl_castable_to_prim(
        BuiltinTypeId bi_type_id, const Value& val) const override;

    TypedValue cast_to_prim(BuiltinTypeId id, RValue&& rval) override;
    RValue cast_to_prim(Ref<PrimType> type, RValue&& rval) override;
};

class IntTypeTpl : public _PrimTypeTpl<IntType> {
    using _PrimTypeTpl::_PrimTypeTpl;
};

} // namespace ulam

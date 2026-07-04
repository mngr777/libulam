#pragma once
#include <libulam/semantic/value/flags.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BoolType : public _PrimType<BoolId, 1, ULAM_MAX_INT_SIZE, 1> {
public:
    BoolType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    TypedValue type_op(TypeOp op) override;

    RValue construct_default(value::flags_t rval_flags = RValue::DefaultFlags) override;
    RValue construct(bool value, value::flags_t rval_flags = RValue::DefaultFlags);

    bool is_true(const RValue& rval) const;

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
    bool is_castable_to_prim(BuiltinTypeId id, bool expl = true) const override;

    TypedValue cast_to_prim(BuiltinTypeId id, RValue&& value) override;
    RValue cast_to_prim(Ref<PrimType> type, RValue&& value) override;
};

class BoolTypeTpl : public _PrimTypeTpl<BoolType> {
    using _PrimTypeTpl::_PrimTypeTpl;
};

} // namespace ulam

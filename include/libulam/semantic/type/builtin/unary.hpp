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

class UnaryType : public _PrimType<UnaryId, 1, ULAM_MAX_INT_SIZE, 32> {
public:
    UnaryType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue construct() const override { return RValue{Unsigned{}}; }

    RValue from_datum(Datum datum) const override;
    Datum to_datum(const RValue& rval) const override;

    TypedValue cast_to(BuiltinTypeId id, RValue&& value) override;
    RValue cast_to(Ref<const PrimType> type, RValue&& value) override;

    TypedValue unary_op(Op op, RValue&& rval) override;

    TypedValue binary_op(
        Op op,
        RValue&& left_rval,
        Ref<const PrimType> right_type,
        RValue&& right_rval) override;

protected:
    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;

    bool is_castable_to_prim(BuiltinTypeId id, bool expl = true) const override;
};

using UnaryTypeTpl = _PrimTypeTpl<UnaryType>;

} // namespace ulam

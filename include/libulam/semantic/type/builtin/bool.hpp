#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BoolType : public _PrimType<BoolId, 1, ULAM_MAX_INT_SIZE, 3> {
public:
    BoolType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue construct() override { return Unsigned{}; }

    RValue from_datum(Datum datum) const override;
    Datum to_datum(const RValue& rval) const override;

    bool is_castable_to(BuiltinTypeId id, bool expl = true) const override;
    bool is_castable_to(Ref<PrimType> type, bool expl = true) const override;

    PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) override;
    RValue cast_to(Ref<PrimType> type, RValue&& value) override;
};

using BoolTypeTpl = _PrimTypeTpl<BoolType>;

} // namespace ulam

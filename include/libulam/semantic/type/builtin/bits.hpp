#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BitsType : public _PrimType<BitsId, 1, 4096, 32> {
public:
    BitsType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue load(const BitsView data, bitsize_t off) const override;
    void store(BitsView data, bitsize_t off, const RValue& rval) const override;

    RValue construct() const override;

    RValue construct(Bits&& bits) const;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;

    Value cast_to(Ref<const Type> type, Value&& val) override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    TypedValue binary_op(
        Op op,
        RValue&& l_rval,
        Ref<const PrimType> r_type,
        RValue&& r_rval) override;

protected:
    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;
    bool is_castable_to_prim(BuiltinTypeId id, bool expl = true) const override;

    TypedValue cast_to_prim(BuiltinTypeId id, RValue&& rval) override;
    RValue cast_to_prim(Ref<const PrimType> type, RValue&& rval) override;
};

class BitsTypeTpl : public _PrimTypeTpl<BitsType> {
    using _PrimTypeTpl::_PrimTypeTpl;
};

} // namespace ulam

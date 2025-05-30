#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BitsType : public _PrimType<BitsId, 1, 4096, 32> {
public:
    using _PrimType::is_castable_to;

    BitsType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    RValue construct() override;

    RValue construct(Bits&& bits);

    bool is_castable_to(
        Ref<const Type> type,
        const Value& value,
        bool expl = true) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    TypedValue unary_op(Op op, RValue&& rval) override;

    TypedValue binary_op(
        Op op,
        RValue&& l_rval,
        Ref<const PrimType> r_type,
        RValue&& r_rval) override;

protected:
    using PrimType::is_impl_castable_to_prim;

    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;
    bool is_castable_to_prim(BuiltinTypeId id, bool expl = true) const override;

    bool
    is_impl_castable_to_prim(Ref<const PrimType> type, const Value& val) const override;

    TypedValue cast_to_prim(BuiltinTypeId id, RValue&& rval) override;
    RValue cast_to_prim(Ref<PrimType> type, RValue&& rval) override;
};

class BitsTypeTpl : public _PrimTypeTpl<BitsType> {
    using _PrimTypeTpl::_PrimTypeTpl;
};

} // namespace ulam

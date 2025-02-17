#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BitsType : public _PrimType<BitsId, 1, 4096, 8> {
public:
    BitsType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue load(const BitVectorView data, BitVector::size_t off) const override;
    void store(BitVectorView data, BitVector::size_t off, const RValue& rval)
        const override;

    RValue construct() override { return RValue{Bits{bitsize()}}; }

    bool is_castable_to(BuiltinTypeId id, bool expl = true) const override;
    bool is_castable_to(Ref<const PrimType> type, bool expl = true) const override;

    PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) override;
    RValue cast_to(Ref<PrimType> type, RValue&& value) override;

    PrimTypedValue binary_op(
        Op op,
        Value&& left_val,
        Ref<const PrimType> right_type,
        Value&& right_val) override;
};

using BitsTypeTpl = _PrimTypeTpl<BitsType>;

} // namespace ulam

#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Diag;
class Value;

class IntType : public _PrimType<IntId, 2, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(TypeIdGen& id_gen, Ref<PrimTypeTpl> tpl, bitsize_t bitsize):
        _PrimType{id_gen, tpl, bitsize} {}

    bool is_convertible(Ref<const Type> type) override;

    Value cast(
        Diag& diag,
        ast::Ref<ast::Node> node,
        Ref<const Type> type,
        const Value& value,
        bool is_impl = true) override;

    ExprRes binary_op(
        Diag& diag,
        ast::Ref<ast::BinaryOp> node,
        Value& left,
        Ref<const Type> right_type,
        const Value& right) override;

private:
    ExprRes binary_op_int(
        Diag& diag,
        ast::Ref<ast::BinaryOp> node,
        Value& left,
        Ref<const PrimType> right_type,
        const Value& right);
};

using IntTypeTpl = _PrimTypeTpl<IntType>;

} // namespace ulam

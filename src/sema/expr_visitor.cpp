#include "libulam/semantic/expr_res.hpp"
#include "libulam/semantic/type/builtin_type_id.hpp"
#include <cassert>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::sema {

ExprRes ExprVisitor::visit(Ref<ast::TypeOpExpr> node) { return {}; }

ExprRes ExprVisitor::visit(Ref<ast::Ident> node) {
    auto name = node->name();
    auto sym = _scope->get(name.str_id());
    if (!sym) {
        diag().emit(
            Diag::Error, node->loc_id(), str(name.str_id()).size(),
            "symbol not found");
        return {ExprError::SymbolNotFound};
    }
    if (sym->is<Var>()) {
        auto var = sym->get<Var>();
        return {var->type(), LValue{var}};
    } else {
        assert(sym->is<FunSet>());
        auto fset = sym->get<FunSet>();
        return {builtins().type(FunId), LValue{fset}};
    }
    assert(false);
}

ExprRes ExprVisitor::visit(Ref<ast::ParenExpr> node) {
    return node->inner()->accept(*this);
}

ExprRes ExprVisitor::visit(Ref<ast::BinaryOp> node) {
    assert(node->has_lhs() && node->has_rhs());
    auto left = node->lhs()->accept(*this);
    auto right = node->rhs()->accept(*this);
    if (!left || !right)
        return {ExprError::Error};
    if (left.type()->is_prim()) {
        if (!right.type()->is_prim()) {
            diag().emit(
                Diag::Error, node->rhs()->loc_id(), 1,
                "non-primitive type cannot be used as right operand type");
            return {ExprError::InvalidOperandType};
        }
        // perform op on prim types
        auto op_res = prim_binary_op(node->op(), left, right);

        // success?
        if (op_res.res().ok())
            return op_res.move_res();

        // emit errors
        auto emit_error = [&](Ref<ast::Expr> node, PrimOpTypeError error) {
            auto message =
                (error.error == PrimOpTypeError::CastSuggested)
                    ? (std::string("suggest explicit casting to ") +
                       std::string{builtin_type_str(error.suggested_type_id)})
                    : "type cannot be used as operand";
            diag().emit(Diag::Error, node->loc_id(), 1, message);
        };
        if (!op_res.errors[0].ok())
            emit_error(node->lhs(), op_res.errors[0]);
        if (!op_res.errors[1].ok())
            emit_error(node->lhs(), op_res.errors[1]);

        // retry with suggested cast?
        if (op_res.res().error() == ExprError::CastRequired) {
            if (!op_res.errors[0].ok()) {
                auto error = op_res.errors[0];
                assert(error.error == PrimOpTypeError::CastSuggested);
                assert(left.type()->as_prim()->is_castable(error.suggested_type_id));
            }
            if (!op_res.errors[1].ok()) {

            }
        }
    }
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryPreOp> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::UnaryPostOp> node) { return {}; }

ExprRes ExprVisitor::visit(Ref<ast::Cast> node) {
    // auto res = node->expr()->accept(*this);
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::BoolLit> node) {
    // Bool(1)
    auto type = builtins().prim_type_tpl(BoolId)->type(diag(), node, 1);
    assert(type);
    return {type, Value{RValue{(Unsigned)node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::NumLit> node) {
    const auto& number = node->value();
    if (number.is_signed()) {
        // Int(n)
        auto tpl = builtins().prim_type_tpl(IntId);
        auto type = tpl->type(diag(), node, number.bitsize());
        assert(type);
        return {type, RValue{number.value<Integer>()}};
    } else {
        // Unsigned(n)
        auto tpl = builtins().prim_type_tpl(UnsignedId);
        auto type = tpl->type(diag(), node, number.bitsize());
        assert(type);
        return {type, RValue{number.value<Unsigned>()}};
    }
}

ExprRes ExprVisitor::visit(Ref<ast::StrLit> node) {
    // String
    auto type = builtins().prim_type(StringId);
    assert(type);
    return {type, Value{RValue{node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::FunCall> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::MemberAccess> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::ArrayAccess> node) { return {}; }

ExprRes ExprVisitor::cast(
    ExprRes&& res, Ref<ast::Node> node, Ref<Type> type, bool is_impl) {
    // ok?
    if (!res.ok())
        return std::move(res);
    assert(res.type());
    // already same type?
    if (res.type() == type)
        return std::move(res);
    Value value =
        res.type()->cast(_program->diag(), node, type, res.value(), is_impl);
    ExprError error = !value.is_nil() ? ExprError::Ok : ExprError::InvalidCast;
    return {type, std::move(value), error};
}

PrimBinaryOpRes ExprVisitor::prim_binary_op(Op op, ExprRes& left, ExprRes& right) {
    assert(left.type()->is_prim() && right.type()->is_prim());
    Ref<PrimType> left_type = left.type()->as_prim();
    Ref<PrimType> right_type = right.type()->as_prim();
    return binary_op(
        op, left_type, left.value(), right_type,
        right.value());
}

Diag& ExprVisitor::diag() { return _program->diag(); }
Builtins& ExprVisitor::builtins() { return _program->builtins(); }

std::string_view ExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema

#include "libulam/semantic/value.hpp"
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

ExprRes ExprVisitor::visit(ast::Ref<ast::TypeOpExpr> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::Ident> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::ParenExpr> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::BinaryOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::UnaryPreOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::UnaryPostOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::VarRef> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::Cast> node) { return {}; }

ExprRes ExprVisitor::visit(ast::Ref<ast::BoolLit> node) {
    // Bool(1)
    auto type = program()->prim_type_tpl(BoolId)->type(node, 1);
    assert(type);
    return {type, Value{RValue{(Unsigned)node->value()}}};
}

ExprRes ExprVisitor::visit(ast::Ref<ast::NumLit> node) {
    const auto& number = node->value();
    if (number.is_signed()) {
        // Int
        auto type =
            program()->prim_type_tpl(IntId)->type(node, number.bitsize());
        assert(type);
        return {type, Value{RValue{number.value<Integer>()}}};
    } else {
        // Unsigned
        auto type =
            program()->prim_type_tpl(UnsignedId)->type(node, number.bitsize());
        assert(type);
        return {type, Value{RValue{number.value<Unsigned>()}}};
    }
}

ExprRes ExprVisitor::visit(ast::Ref<ast::StrLit> node) {
    // String
    auto type = program()->prim_type(StringId);
    assert(type);
    return {type, Value{RValue{node->value()}}};
}

ExprRes ExprVisitor::visit(ast::Ref<ast::FunCall> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::MemberAccess> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::ArrayAccess> node) { return {}; }

} // namespace ulam::sema

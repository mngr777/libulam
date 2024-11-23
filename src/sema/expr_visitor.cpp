#include <libulam/sema/expr_visitor.hpp>

namespace ulam::sema {

ExprRes ExprVisitor::visit(ast::Ref<ast::TypeOpExpr> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::Ident> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::ParenExpr> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::BinaryOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::UnaryPreOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::UnaryPostOp> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::VarRef> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::Cast> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::BoolLit> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::NumLit> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::StrLit> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::FunCall> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::MemberAccess> node) { return {}; }
ExprRes ExprVisitor::visit(ast::Ref<ast::ArrayAccess> node) { return {}; }

} // namespace ulam::sema

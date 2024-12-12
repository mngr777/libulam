#include <cassert>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::sema {

ExprRes ExprVisitor::visit(Ref<ast::TypeOpExpr> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::Ident> node) { return {}; }

ExprRes ExprVisitor::visit(Ref<ast::ParenExpr> node) {
    return node->inner()->accept(*this);
}

ExprRes ExprVisitor::visit(Ref<ast::BinaryOp> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::UnaryPreOp> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::UnaryPostOp> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::VarRef> node) { return {}; }

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

Ref<Program> ExprVisitor::program() {
    assert(ast()->program());
    return ast()->program();
}

Ref<ast::Root> ExprVisitor::ast() {
    assert(_ast);
    return _ast;
}

Diag& ExprVisitor::diag() { return program()->diag(); }

Builtins& ExprVisitor::builtins() { return program()->builtins(); }

} // namespace ulam::sema

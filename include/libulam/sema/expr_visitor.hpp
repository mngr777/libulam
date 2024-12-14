#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {
class Program;
}

namespace ulam::sema {

class ExprVisitor : public ast::ExprVisitor {
public:
    ExprVisitor(Ref<ast::Root> ast, Ref<Scope> scope):
        _ast{ast}, _scope{scope} {}

    ExprVisitor(ExprVisitor&&) = default;
    ExprVisitor& operator=(ExprVisitor&& other);

    virtual ExprRes visit(Ref<ast::TypeOpExpr> node) override;
    virtual ExprRes visit(Ref<ast::Ident> node) override;
    virtual ExprRes visit(Ref<ast::ParenExpr> node) override;
    virtual ExprRes visit(Ref<ast::BinaryOp> node) override;
    virtual ExprRes visit(Ref<ast::UnaryPreOp> node) override;
    virtual ExprRes visit(Ref<ast::UnaryPostOp> node) override;
    virtual ExprRes visit(Ref<ast::VarRef> node) override;
    virtual ExprRes visit(Ref<ast::Cast> node) override;
    virtual ExprRes visit(Ref<ast::BoolLit> node) override;
    virtual ExprRes visit(Ref<ast::NumLit> node) override;
    virtual ExprRes visit(Ref<ast::StrLit> node) override;
    virtual ExprRes visit(Ref<ast::FunCall> node) override;
    virtual ExprRes visit(Ref<ast::MemberAccess> node) override;
    virtual ExprRes visit(Ref<ast::ArrayAccess> node) override;

protected:
    Ref<Program> program();
    Ref<ast::Root> ast();
    Diag& diag();
    Builtins& builtins();

private:
    Ref<ast::Root> _ast;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

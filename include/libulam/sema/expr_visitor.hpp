#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
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
    ExprVisitor(Ref<Program> program, Ref<Scope> scope):
        _program{program}, _scope{scope} {}

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
    Diag& diag();
    Builtins& builtins();

private:
    Ref<Program> _program;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

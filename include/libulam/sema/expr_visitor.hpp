#pragma once
#include <cassert>
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {
class Diag;
class Program;
} // namespace ulam

namespace ulam::sema {

class ExprVisitor : public ast::ExprVisitor {
public:
    ExprVisitor(Diag& diag, ast::Ref<ast::Root> ast, Scope* scope):
        _diag{diag}, _ast{ast}, _scope{scope} {}

    virtual ExprRes visit(ast::Ref<ast::TypeOpExpr> node) override;
    virtual ExprRes visit(ast::Ref<ast::Ident> node) override;
    virtual ExprRes visit(ast::Ref<ast::ParenExpr> node) override;
    virtual ExprRes visit(ast::Ref<ast::BinaryOp> node) override;
    virtual ExprRes visit(ast::Ref<ast::UnaryPreOp> node) override;
    virtual ExprRes visit(ast::Ref<ast::UnaryPostOp> node) override;
    virtual ExprRes visit(ast::Ref<ast::VarRef> node) override;
    virtual ExprRes visit(ast::Ref<ast::Cast> node) override;
    virtual ExprRes visit(ast::Ref<ast::BoolLit> node) override;
    virtual ExprRes visit(ast::Ref<ast::NumLit> node) override;
    virtual ExprRes visit(ast::Ref<ast::StrLit> node) override;
    virtual ExprRes visit(ast::Ref<ast::FunCall> node) override;
    virtual ExprRes visit(ast::Ref<ast::MemberAccess> node) override;
    virtual ExprRes visit(ast::Ref<ast::ArrayAccess> node) override;

protected:
    Ref<Program> program() {
        assert(ast()->program());
        return ast()->program();
    }
    ast::Ref<ast::Root> ast() {
        assert(_ast);
        return _ast;
    }

private:
    Diag& _diag;
    ast::Ref<ast::Root> _ast;
    Scope _scope;
};

} // namespace ulam::sema

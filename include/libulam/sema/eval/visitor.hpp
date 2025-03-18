#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope/stack.hpp>

namespace ulam::sema {

class EvalExprVisitor;

class EvalVisitor : public ast::Visitor {
    friend EvalExprVisitor;

public:
    EvalVisitor(Ref<Program> program);

    ExprRes eval(Ref<ast::Block> block);

    void visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;

    void visit(Ref<ast::Block> node) override;
    void visit(Ref<ast::FunDefBody>) override;
    void visit(Ref<ast::If> node) override;
    void visit(Ref<ast::IfAs> node) override;
    void visit(Ref<ast::For> node) override;
    void visit(Ref<ast::While> node) override;
    void visit(Ref<ast::Which> node) override;
    void visit(Ref<ast::Return> node) override;
    void visit(Ref<ast::Break> node) override;
    void visit(Ref<ast::Continue> node) override;
    void visit(Ref<ast::ExprStmt> node) override;
    void visit(Ref<ast::UnaryOp> node) override;
    void visit(Ref<ast::BinaryOp> node) override;
    void visit(Ref<ast::FunCall> node) override;
    void visit(Ref<ast::ArrayAccess> node) override;
    void visit(Ref<ast::MemberAccess> node) override;
    void visit(Ref<ast::TypeOpExpr> node) override;
    void visit(Ref<ast::Ident> node) override;

protected:
    virtual ExprRes
    funcall(Ref<Fun> fun, LValue self, TypedValueList&& args);

private:
    ExprRes eval_expr(Ref<ast::Expr> expr);
    bool eval_cond(Ref<ast::Expr> expr);

    Ref<Scope> scope() { return _scope_stack.top(); }

    Builtins& builtins() { return _program->builtins(); }
    Diag& diag() { return _program->diag(); }

    std::string_view str(str_id_t str_id) {
        return _program->str_pool().get(str_id);
    }

    Ref<Program> _program;
    Resolver _resolver;
    ScopeStack _scope_stack;
};

} // namespace ulam::sema

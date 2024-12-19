#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/sema/cast.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope/stack.hpp>

namespace ulam::sema {

class EvalVisitor : public ast::Visitor {
public:
    EvalVisitor(Ref<Program> program);

    ExprRes eval(Ref<ast::Block> block);

    void visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;

    void visit(Ref<ast::Block> node) override;
    void visit(Ref<ast::If> node) override;
    void visit(Ref<ast::For> node) override;
    void visit(Ref<ast::While> node) override;

private:
    ExprRes eval_expr(Ref<ast::Expr> expr);
    bool eval_cond(Ref<ast::Expr> expr);

    Ref<Scope> scope() { return _scope_stack.top(); }

    Ref<Program> _program;
    Resolver _resolver;
    Cast _cast;
    ScopeStack _scope_stack;
};

} // namespace ulam::sema

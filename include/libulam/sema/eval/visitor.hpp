#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/stack.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::sema {

class EvalCast;
class EvalExprVisitor;
class EvalInit;
class EvalFuncall;

class EvalVisitor : public ast::Visitor {
public:
    explicit EvalVisitor(
        Ref<Program> program, eval_flags_t flags = evl::NoFlags);

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

    Ptr<Resolver> resolver(eval_flags_t flags = evl::NoFlags);

    Ptr<EvalExprVisitor>
    expr_visitor(Ref<Scope> scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalInit>
    init_helper(Ref<Scope> scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalCast>
    cast_helper(Ref<Scope> scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalFuncall>
    funcall_helper(Ref<Scope> scope, eval_flags_t flags = evl::NoFlags);

    virtual ExprRes funcall(Ref<Fun> fun, LValue self, ExprResList&& args);

protected:
    virtual Ptr<Resolver> _resolver(eval_flags_t flags);

    virtual Ptr<EvalExprVisitor>
    _expr_visitor(Ref<Scope> scope, eval_flags_t flags);

    virtual Ptr<EvalInit> _init_helper(Ref<Scope> scope, eval_flags_t flags);

    virtual Ptr<EvalCast> _cast_helper(Ref<Scope> scope, eval_flags_t flags);

    virtual Ptr<EvalFuncall>
    _funcall_helper(Ref<Scope> scope, eval_flags_t flags);

    virtual Ref<AliasType> type_def(Ref<ast::TypeDef> node);

    virtual Ref<Var>
    var_def(Ref<ast::TypeName> type_name, Ref<ast::VarDef> node);

    virtual Ptr<Var>
    make_var(Ref<ast::TypeName> type_name, Ref<ast::VarDef> node);

    virtual void var_set_init(Ref<Var> var, ExprRes&& init);
    virtual void var_set_default(Ref<Var> var);

    virtual ExprRes ret_res(Ref<ast::Return> node);

    ExprRes eval_expr(Ref<ast::Expr> expr, eval_flags_t flags = evl::NoFlags);
    virtual ExprRes _eval_expr(Ref<ast::Expr> expr, eval_flags_t flags);

    bool eval_cond(Ref<ast::Expr> expr, eval_flags_t flags = evl::NoFlags);
    virtual bool _eval_cond(Ref<ast::Expr> expr, eval_flags_t flags);

    Ref<Scope> scope() { return _scope_stack.top(); }

    Builtins& builtins() { return _program->builtins(); }
    Diag& diag() { return _program->diag(); }

    std::string_view str(str_id_t str_id) {
        return _program->str_pool().get(str_id);
    }

    Ref<Program> _program;
    eval_flags_t _flags;
    EvalStack _stack;
    ScopeStack _scope_stack;
};

} // namespace ulam::sema

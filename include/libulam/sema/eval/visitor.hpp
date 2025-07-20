#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/stack.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::sema {

class EvalCast;
class EvalExprVisitor;
class EvalInit;
class EvalFuncall;

class EvalVisitor : public ast::Visitor, public EvalBase {
    friend Resolver;

public:
    explicit EvalVisitor(
        Ref<Program> program, eval_flags_t flags = evl::NoFlags);

    virtual ExprRes eval(Ref<ast::Block> block);

    void visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;

    void visit(Ref<ast::Block> node) override;
    void visit(Ref<ast::FunDefBody>) override;
    void visit(Ref<ast::If> node) override;
    void visit(Ref<ast::For> node) override;
    void visit(Ref<ast::While> node) override;
    void visit(Ref<ast::Which> node) override;
    void visit(Ref<ast::Return> node) override;
    void visit(Ref<ast::Break> node) override;
    void visit(Ref<ast::Continue> node) override;
    void visit(Ref<ast::ExprStmt> node) override;
    void visit(Ref<ast::EmptyStmt> node) override;
    void visit(Ref<ast::UnaryOp> node) override;
    void visit(Ref<ast::BinaryOp> node) override;
    void visit(Ref<ast::FunCall> node) override;
    void visit(Ref<ast::ArrayAccess> node) override;
    void visit(Ref<ast::MemberAccess> node) override;
    void visit(Ref<ast::TypeOpExpr> node) override;
    void visit(Ref<ast::Ident> node) override;

    Ptr<Resolver> resolver(bool in_expr, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalExprVisitor>
    expr_visitor(Scope* scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalInit> init_helper(Scope* scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalCast> cast_helper(Scope* scope, eval_flags_t flags = evl::NoFlags);

    Ptr<EvalFuncall>
    funcall_helper(Scope* scope, eval_flags_t flags = evl::NoFlags);

    virtual ExprRes funcall(Ref<Fun> fun, LValue self, ExprResList&& args);

protected:
    class EvalCondContext {
    public:
        EvalCondContext(
            Ref<Type> type, Ptr<ast::VarDef>&& var_def, Ref<Var> var):
            _type{type}, _var_def{std::move(var_def)}, _var{var} {}

        EvalCondContext(Ref<Type> type): EvalCondContext{type, {}, {}} {}

        EvalCondContext(): EvalCondContext{{}} {}

        Ref<Type> type() { return _type; }
        Ref<Var> var() { return _var; }

    private:
        Ref<Type> _type;
        Ptr<ast::VarDef> _var_def;
        Ref<Var> _var{};
    };

    using EvalCondRes = std::pair<ExprRes, EvalCondContext>;

    virtual Ptr<Resolver> _resolver(bool in_expr, eval_flags_t flags);

    virtual Ptr<EvalExprVisitor>
    _expr_visitor(Scope* scope, eval_flags_t flags);

    virtual Ptr<EvalInit> _init_helper(Scope* scope, eval_flags_t flags);

    virtual Ptr<EvalCast> _cast_helper(Scope* scope, eval_flags_t flags);

    virtual Ptr<EvalFuncall> _funcall_helper(Scope* scope, eval_flags_t flags);

    virtual Ref<AliasType> type_def(Ref<ast::TypeDef> node);

    virtual Ref<Var>
    var_def(Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual Ptr<Var> make_var(
        Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual void var_init_expr(Ref<Var> var, ExprRes&& init, bool in_expr);
    virtual void var_init_default(Ref<Var> var, bool in_expr);
    virtual void var_init(Ref<Var> var, bool in_expr);

    virtual Ptr<Var> make_which_tmp_var(Ref<ast::Which> node);
    virtual ExprRes eval_which_expr(Ref<ast::Which> node);
    virtual ExprRes eval_which_match(
        Ref<ast::Expr> expr,
        Ref<ast::Expr> case_expr,
        ExprRes&& expr_res,
        ExprRes&& case_res);
    virtual std::optional<bool>
    which_match(Ref<ast::Expr> expr, Ref<ast::Expr> case_expr, Ref<Var> var);

    virtual ExprRes ret_res(Ref<ast::Return> node);

    ExprRes eval_expr(Ref<ast::Expr> expr, eval_flags_t flags = evl::NoFlags);
    virtual ExprRes _eval_expr(Ref<ast::Expr> expr, eval_flags_t flags);

    virtual EvalCondRes eval_cond(
        Ref<ast::Cond> cond, Scope* scope, eval_flags_t flags = evl::NoFlags);

    virtual EvalCondRes
    eval_as_cond(Ref<ast::UnaryOp> as_cond, Scope* scope, eval_flags_t flags);

    ExprRes
    eval_cond_expr(Ref<ast::Expr> expr, eval_flags_t flags = evl::NoFlags);
    virtual ExprRes _eval_cond_expr(Ref<ast::Expr> expr, eval_flags_t flags);

    virtual ExprRes eval_as_cond_ident(Ref<ast::Ident> ident);
    virtual Ref<Type> resolve_as_cond_type(Ref<ast::TypeName> type_name);
    virtual std::pair<Ptr<ast::VarDef>, Ref<Var>> define_as_cond_var(
        Ref<ast::UnaryOp> node, ExprRes&& res, Ref<Type> type, Scope* scope);

    ExprRes to_boolean(
        Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags = evl::NoFlags);
    virtual ExprRes
    _to_boolean(Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags);

    bool is_true(const ExprRes& res);

    Scope* scope() { return _scope_stack.top(); }

    EvalStack _stack;
    BasicScope _program_scope;
    ScopeStack _scope_stack;
};

} // namespace ulam::sema

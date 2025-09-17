#pragma once
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/stack.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::sema {

class EvalCast;
class EvalCond;
class EvalExprVisitor;
class EvalFuncall;
class EvalInit;
class EvalVisitor;
class EvalWhich;

class EvalEnv : public EvalBase {
public:
    using StackRaii = EvalStack::Raii;
    using ScopeRaii = ScopeStack::Raii<BasicScope>;

    class ScopeSwitchRaii {
        friend EvalEnv;

    public:
        ScopeSwitchRaii();
        ~ScopeSwitchRaii();

        ScopeSwitchRaii(ScopeSwitchRaii&& other);
        ScopeSwitchRaii& operator=(ScopeSwitchRaii&& other);

    private:
        ScopeSwitchRaii(EvalEnv& env, Scope* scope);

        EvalEnv* _env;
        Scope* _old_scope_override;
    };

    class FlagsRaii {
        friend EvalEnv;

    public:
        FlagsRaii();
        ~FlagsRaii();

        FlagsRaii(FlagsRaii&& other);
        FlagsRaii& operator=(FlagsRaii&& other);

    private:
        FlagsRaii(EvalEnv& env, eval_flags_t flags);

        EvalEnv* _env;
        eval_flags_t _old_flags;
    };

    EvalEnv(Ref<Program> program, eval_flags_t flags = evl::NoFlags);

    EvalEnv(EvalEnv&&) = default;
    EvalEnv& operator=(EvalEnv&&) = default;

    Resolver resolver(bool in_expr);

    virtual ExprRes eval(Ref<ast::Block> block);

    virtual void eval_stmt(Ref<ast::Stmt> stmt);

    virtual void eval_which(Ref<ast::Which> which);

    virtual ExprRes eval_expr(Ref<ast::Expr> expr);

    virtual ExprRes eval_equal(
        Ref<ast::Expr> node,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

    virtual CondRes eval_cond(Ref<ast::Cond> cond);

    virtual ExprRes
    cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl = false);

    virtual ExprRes cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl = false);

    virtual ExprRes cast_to_idx(Ref<ast::Node> node, ExprRes&& arg);

    virtual ExprRes to_boolean(Ref<ast::Expr> expr, ExprRes&& arg);

    virtual bool init_var(Ref<Var> var, Ref<ast::InitValue> init, bool in_expr);

    virtual bool init_prop(Ref<Prop> prop, Ref<ast::InitValue> init);

    virtual ExprRes
    construct(Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args);

    virtual ExprRes
    call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args);

    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args);

    StackRaii stack_raii(Ref<Fun> fun, LValue self);

    ScopeRaii scope_raii(scope_flags_t flags = scp::NoFlags);
    ScopeRaii scope_raii(Scope* parent, scope_flags_t flags = scp::NoFlags);

    ScopeSwitchRaii scope_switch_raii(Scope* scope);

    FlagsRaii flags_raii(eval_flags_t flags);
    FlagsRaii add_flags_raii(eval_flags_t flags);
    FlagsRaii remove_flags_raii(eval_flags_t flags);

    const EvalStack::Item& stack_top() const;
    std::size_t stack_size() const;

    Scope* scope();
    scope_lvl_t scope_lvl() const;

    eval_flags_t flags() const;
    bool has_flag(eval_flags_t flag) const;

protected:
    virtual void do_eval_stmt(EvalVisitor& vis, Ref<ast::Stmt> stmt);

    virtual void do_eval_which(EvalWhich& ew, Ref<ast::Which> which);

    virtual ExprRes do_eval_expr(EvalExprVisitor& ev, Ref<ast::Expr> expr);

    virtual ExprRes do_eval_equal(
        EvalExprVisitor& ev,
        Ref<ast::Expr> node,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

    virtual CondRes do_eval_cond(EvalCond& ec, Ref<ast::Cond> cond);

    virtual ExprRes do_cast(
        EvalCast& ec,
        Ref<ast::Node> node,
        Ref<Type> type,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes do_cast(
        EvalCast& ec,
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes
    do_cast_to_idx(EvalCast& ec, Ref<ast::Node> node, ExprRes&& arg);

    virtual bool do_init_var(
        EvalInit& ei, Ref<Var> var, Ref<ast::InitValue> init, bool in_expr);

    virtual bool
    do_init_prop(EvalInit& ei, Ref<Prop> prop, Ref<ast::InitValue> init);

    virtual ExprRes do_construct(
        EvalFuncall& ef,
        Ref<ast::Node> node,
        Ref<Class> cls,
        ExprResList&& args);

    virtual ExprRes do_call(
        EvalFuncall& ef,
        Ref<ast::Node> node,
        ExprRes&& callable,
        ExprResList&& args);

    virtual ExprRes do_funcall(
        EvalFuncall& ef,
        Ref<ast::Node> node,
        Ref<Fun> fun,
        ExprRes&& obj,
        ExprResList&& args);

private:
    Ref<Program> _program;
    BasicScope _program_scope;
    EvalStack _stack;
    ScopeStack _scope_stack;
    Scope* _scope_override{};
    eval_flags_t _flags;
};

} // namespace ulam::sema

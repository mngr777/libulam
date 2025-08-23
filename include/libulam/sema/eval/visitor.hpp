#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/cond_res.hpp>
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
class EvalFuncall;
class EvalInit;

class EvalVisitor : public ast::Visitor, public EvalBase {
    friend FlagsRaii;
    friend ScopeRaii;
    friend Resolver; // TODO: remove

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

    Resolver resolver(bool in_expr);

    virtual ExprRes eval_expr(Ref<ast::Expr> expr);

    virtual ExprRes eval_equal(
        Ref<ast::Expr> node,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

    virtual ExprRes
    cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl = false);

    virtual ExprRes cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl = false);

    virtual ExprRes to_boolean(Ref<ast::Expr> expr, ExprRes&& arg);

    virtual ExprRes eval_init(Ref<VarBase> var, Ref<ast::InitValue> init);

    virtual ExprRes
    construct(Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args);

    virtual ExprRes
    call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args);

    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args);

    // TODO: move to helper
    virtual ExprRes old_funcall(Ref<Fun> fun, LValue self, ExprResList&& args);

    FlagsRaii flags_raii(eval_flags_t flags) { return {*this, flags}; }

    ScopeRaii scope_raii(Scope* scope) { return {*this, scope}; }

    bool has_flag(eval_flags_t flag) const { return _flags & flag; }
    eval_flags_t flags() const { return _flags; }

    Scope* scope();

protected:
    virtual ExprRes do_eval_expr(EvalExprVisitor& ev, Ref<ast::Expr> expr);

    virtual ExprRes do_eval_equal(
        EvalExprVisitor& ev,
        Ref<ast::Expr> node,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

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
    do_eval_init(EvalInit& ei, Ref<VarBase> var, Ref<ast::InitValue> init);

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

    virtual CondRes eval_cond(Ref<ast::Cond> cond);

    virtual Ref<AliasType> type_def(Ref<ast::TypeDef> node);

    virtual Ref<Var>
    var_def(Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual Ptr<Var> make_var(
        Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual void var_init_expr(Ref<Var> var, ExprRes&& init, bool in_expr);
    virtual void var_init_default(Ref<Var> var, bool in_expr);
    virtual void var_init(Ref<Var> var, bool in_expr);

    virtual ExprRes ret_res(Ref<ast::Return> node);

private:
    Scope* _scope{};
    EvalStack _stack;
    BasicScope _program_scope;
    ScopeStack _scope_stack;
    eval_flags_t _flags;
};

} // namespace ulam::sema

#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/exprs.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/scope/stack.hpp>

namespace ulam::sema {

class EvalCond;

class EvalWhich : public EvalHelper {
public:
    using EvalHelper::EvalHelper;

    virtual void eval_which(Ref<ast::Which> node);

protected:
    struct Context {
        Context(Ref<ast::Which> node): node{node} {}

        Ref<ast::Which> node;
        bool matched{false};
        Ptr<Var> which_var{};
        AsCondContext as_cond_ctx;
    };

    virtual Ptr<Var> make_which_var(Context& ctx, Ref<ast::Expr> expr);

    virtual void eval_cases(Context& ctx);

    virtual void eval_case(Context& ctx, Ref<ast::WhichCase> case_);

    virtual bool match(Context& ctx, Ref<ast::WhichCaseCond> case_cond);

    virtual bool match_expr(Context& ctx, Ref<ast::Expr> case_expr);

    virtual bool match_as_cond(Context& ctx, Ref<ast::AsCond> as_cond);

    virtual bool
    do_match_as_cond(Context& ctx, EvalCond& ec, Ref<ast::AsCond> as_cond);

    virtual ExprRes
    match_expr_res(Context& ctx, Ref<ast::Expr> case_expr, ExprRes&& case_res);

    virtual ExprRes make_which_expr(Context& ctx);
};

} // namespace ulam::sema

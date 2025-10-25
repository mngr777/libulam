#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/which.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalWhich] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

void EvalWhich::eval_which(Ref<ast::Which> node) {
    auto sr = env().scope_raii(scp::Break);
    Context ctx{node};
    if (node->has_expr())
        ctx.which_var = make_which_var(ctx, node->expr());
    try {
        eval_cases(ctx);
    } catch (const EvalExceptBreak&) {
        debug() << "break\n";
    }
}

void EvalWhich::eval_cases(Context& ctx) {
    for (unsigned i = 0; i < ctx.node->case_num(); ++i) {
        auto case_ = ctx.node->case_(i);
        eval_case(ctx, case_);
    }
}

Ptr<Var> EvalWhich::make_which_var(Context& ctx, Ref<ast::Expr> expr) {
    auto res = env().eval_expr(expr);
    return make<Var>(res.move_typed_value());
}

void EvalWhich::eval_case(Context& ctx, Ref<ast::WhichCase> case_) {
    auto sr = env().scope_raii();
    if (ctx.matched || match(ctx, case_->case_cond()))
        env().eval_stmt(case_->branch());
}

bool EvalWhich::match(Context& ctx, Ref<ast::WhichCaseCond> case_cond) {
    assert(!ctx.matched);
    if (case_cond->is_default()) {
        ctx.matched = true;
    } else {
        ctx.matched = case_cond->is_as_cond()
                          ? match_as_cond(ctx, case_cond->as_cond())
                          : match_expr(ctx, case_cond->expr());
    }
    return ctx.matched;
}

bool EvalWhich::match_expr(Context& ctx, Ref<ast::Expr> case_expr) {
    auto case_res = env().eval_expr(case_expr);
    if (!case_res)
        throw EvalExceptError("failed to eval which case");
    auto res = match_expr_res(ctx, case_expr, std::move(case_res));
    return is_true(res);
}

bool EvalWhich::match_as_cond(Context& ctx, Ref<ast::AsCond> as_cond) {
    EvalCond ec{env()};
    return do_match_as_cond(ctx, ec, as_cond);
}

bool EvalWhich::do_match_as_cond(
    Context& ctx, EvalCond& ec, Ref<ast::AsCond> as_cond) {
    bool is_match{};
    std::tie(is_match, ctx.as_cond_ctx) = ec.eval_as_cond(as_cond);
    return is_match;
}

ExprRes EvalWhich::match_expr_res(
    Context& ctx, Ref<ast::Expr> case_expr, ExprRes&& case_res) {
    auto which_res = make_which_expr(ctx);
    auto which_expr = ctx.node->expr();
    auto res = env().eval_equal(
        case_expr, which_expr, std::move(which_res), case_expr,
        std::move(case_res));
    if (!res || (!has_flag(evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to match eval which case");
    return env().to_boolean(case_expr, std::move(res));
}

ExprRes EvalWhich::make_which_expr(Context& ctx) {
    return {ctx.which_var->type(), Value{ctx.which_var->lvalue()}};
}

} // namespace ulam::sema

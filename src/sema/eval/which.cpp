#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/visitor.hpp>
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
    auto scope_raii = scope_stack().raii<BasicScope>(scope(), scp::Break);
    Context ctx{node};
    if (node->has_expr())
        ctx.which_var = make_which_var(ctx, node->expr());
    try {
        for (unsigned i = 0; i < node->case_num(); ++i) {
            auto case_ = node->case_(i);
            eval_case(ctx, case_);
        }
    } catch (const EvalExceptBreak&) {
        debug() << "break\n";
    }
}

Ptr<Var> EvalWhich::make_which_var(Context& ctx, Ref<ast::Expr> expr) {
    auto res = eval()->eval_expr(expr);
    return make<Var>(res.move_typed_value());
}

void EvalWhich::eval_case(Context& ctx, Ref<ast::WhichCase> case_) {
    auto scope_raii = scope_stack().raii<BasicScope>(scope());
    if (match(ctx, case_->case_cond()))
        case_->branch()->accept(*eval());
}

bool EvalWhich::match(Context& ctx, Ref<ast::WhichCaseCond> case_cond) {
    if (case_cond->is_default())
        return true;
    return case_cond->is_as_cond() ? match_as_cond(ctx, case_cond->as_cond())
                                   : match_expr(ctx, case_cond->expr());
}

bool EvalWhich::match_expr(Context& ctx, Ref<ast::Expr> case_expr) {
    auto case_res = eval()->eval_expr(case_expr);
    if (!case_res)
        throw EvalExceptError("failed to eval which case");
    auto res = match_expr_res(ctx, case_expr, std::move(case_res));
    return is_true(res);
}

bool EvalWhich::match_as_cond(Context& ctx, Ref<ast::AsCond> as_cond) {
    EvalCond ec{*eval(), program(), scope_stack()};
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
    ExprRes which_res{ctx.which_var->type(), Value{ctx.which_var->lvalue()}};
    auto which_expr = ctx.node->expr();
    auto res = eval()->eval_equal(
        case_expr, case_expr, std::move(case_res), which_expr,
        std::move(which_res));
    if (!res || (!has_flag(evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to match eval which case");
    return eval()->to_boolean(case_expr, std::move(res));
}

} // namespace ulam::sema

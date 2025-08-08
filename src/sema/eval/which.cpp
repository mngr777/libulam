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
    // TODO: public eval().eval_expr()
    auto ev = eval().expr_visitor(scope(), flags());
    auto res = expr->accept(*ev);
    return make<Var>(res.move_typed_value());
}

void EvalWhich::eval_case(Context& ctx, Ref<ast::WhichCase> case_) {
    auto scope_raii = scope_stack().raii<BasicScope>(scope());
    if (match(ctx, case_->case_cond()))
        case_->branch()->accept(eval());
}

bool EvalWhich::match(Context& ctx, Ref<ast::WhichCaseCond> case_cond) {
    if (case_cond->is_default())
        return true;
    return case_cond->is_as_cond() ? match_as_cond(ctx, case_cond->as_cond())
                                   : match_expr(ctx, case_cond->expr());
}

bool EvalWhich::match_expr(Context& ctx, Ref<ast::Expr> case_expr) {
    // TODO: public eval().eval_expr()
    auto ev = eval().expr_visitor(scope(), flags());
    auto case_res = case_expr->accept(*ev);
    if (!case_res)
        throw EvalExceptError("failed to eval which case");
    auto res = match_expr_res(ctx, case_expr, std::move(case_res));
    return is_true(res);
}

bool EvalWhich::match_as_cond(Context& ctx, Ref<ast::AsCond> as_cond) {
    // TODO: use EvalCond
    return false;
}

ExprRes EvalWhich::match_expr_res(
    Context& ctx, Ref<ast::Expr> case_expr, ExprRes&& case_res) {
    ExprRes which_res{ctx.which_var->type(), Value{ctx.which_var->lvalue()}};
    auto ev = eval().expr_visitor(scope(), flags());
    auto which_expr = ctx.node->expr();
    auto res = ev->binary_op(
        case_expr, Op::Equal, case_expr, std::move(case_res), which_expr,
        std::move(which_res));
    if (!res || (!has_flag(evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to match eval which case");
    return to_boolean(scope(), case_expr, std::move(res), flags());
}

} // namespace ulam::sema

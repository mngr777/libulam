#include "./which.hpp"
#include "../out.hpp"
#include "./codegen.hpp"
#include "./codegen/context_stack.hpp"
#include "./expr_res.hpp"
#include "src/semantic/detail/leximited.hpp" // TODO: remove dep
#include <cassert>

using ExprRes = EvalWhich::ExprRes;

void EvalWhich::eval_which(ulam::Ref<ulam::ast::Which> node) {
    gen::ContextStack::Raii cr;
    if (codegen_enabled()) {
        auto label_idx = gen().next_tmp_idx_str();
        auto tmp_idx =
            ulam::detail::leximited((ulam::Unsigned)gen().next_tmp_idx());
        cr = gen().ctx_stack().raii(gen::WhichContext{label_idx, tmp_idx});
        gen().block_open();
    }

    Base::eval_which(node);

    if (codegen_enabled())
        gen().block_close();
}

ulam::Ptr<ulam::Var>
EvalWhich::make_which_var(Context& ctx, ulam::Ref<ulam::ast::Expr> expr) {
    if (!codegen_enabled())
        return Base::make_which_var(ctx, expr);

    auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
    auto res = env().eval_expr(expr);
    assert(res);

    auto strf = gen().make_strf();
    auto type = res.type()->canon();
    auto type_str = out::type_str(strf, type);
    auto type_dim_str = out::type_dim_str(type);

    // tmp typedef
    gen().append("typedef");
    gen().append(type_str);
    gen().append(gen_ctx.tmp_type_name() + type_dim_str + "; ");

    // tmp var def
    gen().append(type_str);
    gen().append(gen_ctx.tmp_var_name() + type_dim_str);
    gen().append("=");
    gen().append(exp::data(res) + "; ");

    return ulam::make<ulam::Var>(res.move_typed_value());
}

void EvalWhich::eval_cases(Context& ctx) {
    Base::eval_cases(ctx);

    if (codegen_enabled()) {
        auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
        if (gen_ctx.non_default_num() > 0)
            gen().append("else");
        if (gen_ctx.has_breaks())
            gen().append("_" + gen_ctx.label_idx() + ":");
    }
}

void EvalWhich::eval_case(Context& ctx, ulam::Ref<ulam::ast::WhichCase> case_) {
    if (!codegen_enabled())
        return Base::eval_case(ctx, case_);

    bool is_default = case_->is_default();
    bool has_branch = case_->has_branch();

    auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
    gen_ctx.set_case_has_breaks(false);
    if (!is_default)
        gen_ctx.inc_non_default_num();

    // TODO: check
    bool fallthru = ctx.matched;
    bool has_cond = fallthru;
    match(ctx, case_->case_cond());
    if (!is_default && has_branch) {
        gen().append(gen_ctx.move_cond_str());
        gen().append("cond");
        has_cond = true;
    } else if (is_default && !gen_ctx.has_non_default()) {
        // default case is the only one, t41038
        gen().append("true");
        gen().append("cond");
        has_cond = true;
    }

    if (has_branch) {
        env().eval_stmt(case_->branch());
        if (!has_cond) {
            gen().append("else");
        } else if (!is_default || !fallthru) {
            gen().append("if");
        }
    }
}

ExprRes EvalWhich::match_expr_res(
    Context& ctx, ulam::Ref<ulam::ast::Expr> case_expr, ExprRes&& case_res) {
    auto res = Base::match_expr_res(ctx, case_expr, std::move(case_res));
    if (codegen_enabled()) {
        auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
        gen_ctx.add_cond(exp::data(res));
    }
    return res;
}

ExprRes EvalWhich::make_which_expr(Context& ctx) {
    auto res = Base::make_which_expr(ctx);
    if (codegen_enabled()) {
        auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
        exp::set_data(res, gen_ctx.tmp_var_name());
    }
    return res;
}

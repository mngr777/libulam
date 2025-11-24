#include "./which.hpp"
#include "../out.hpp"
#include "./codegen.hpp"
#include "./codegen/context_stack.hpp"
#include "./expr_res.hpp"
#include <cassert>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/utils/leximited.hpp>
#include <sstream>

using ExprRes = EvalWhich::ExprRes;

void EvalWhich::eval_which(ulam::Ref<ulam::ast::Which> node) {
    gen::ContextStack::Raii cr;
    if (codegen_enabled()) {
        auto label_idx = gen().next_tmp_idx_str();
        auto tmp_idx =
            ulam::detail::leximited((ulam::Unsigned)gen().next_tmp_idx());
        cr = gen().ctx_stack().raii(gen::WhichContext{label_idx, tmp_idx});
        gen().block_open();

        if (!node->has_expr()) {
            // which-as
            auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
            gen().append("typedef");
            gen().append(std::string{builtins().boolean()->name()});
            gen().append(gen_ctx.tmp_type_name() + "; ");
        }
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

bool EvalWhich::eval_case(Context& ctx, ulam::Ref<ulam::ast::WhichCase> case_) {
    if (!codegen_enabled())
        return Base::eval_case(ctx, case_);

    auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
    gen_ctx.set_case_has_breaks(false);
    bool is_default = match_conds(ctx, case_->conds());

    auto branch = [&]() { env().eval_stmt(case_->branch()); };
    if (!ctx.as_cond_ctx.empty()) {
        auto sr = env().as_cond_scope_raii(ctx.as_cond_ctx);
        branch();
    } else {
        branch();
    }

    if (!is_default) {
        gen().append("if");
    } else {
        if (!gen_ctx.has_non_default()) {
            // t41038, only option
            gen().append("if");
        } else if (case_->conds()->child_num() == 1) {
            // t41021: no "else" if branch has non-default conds
            gen().append("else");
        }
    }
    return false; // not default case
}

// NOTE: repurposing return value to mean "is default"
bool EvalWhich::match_conds(
    Context& ctx, ulam::Ref<ulam::ast::WhichCaseCondList> case_conds) {
    if (!codegen_enabled())
        return Base::match_conds(ctx, case_conds);

    auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();

    for (unsigned n = 0; n < case_conds->child_num(); ++n) {
        if (case_conds->get(n)->is_default()) {
            if (!gen_ctx.has_non_default()) {
                // t41038, only option
                gen().append("true");
                gen().append("cond");
            }
            return true;
        }
    }

    ulam::Ref<ulam::ast::AsCond> as_cond{};
    for (unsigned n = 0; n < case_conds->child_num(); ++n) {
        gen_ctx.inc_non_default_num();
        auto case_cond = case_conds->get(n);
        match(ctx, case_cond);

        // remember ast::AsCond node (not added to AsCondContext yet)
        if (case_cond->is_as_cond())
            as_cond = case_cond->as_cond();
    }
    gen().append(gen_ctx.move_cond_str());
    gen().append("cond");

    if (!ctx.as_cond_ctx.empty()) {
        assert(as_cond);
        auto type = ctx.as_cond_ctx.type();
        gen().set_next_prefix(gen().as_cond_prefix_str(as_cond, type));
    }

    return false;
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

bool EvalWhich::do_match_as_cond(
    Context& ctx, EvalCond& ec, ulam::Ref<ulam::ast::AsCond> as_cond) {
    Base::do_match_as_cond(ctx, ec, as_cond);
    if (codegen_enabled()) {
        auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
        auto type = ctx.as_cond_ctx.type();
        gen_ctx.add_cond(gen().as_cond_str(as_cond, type));
    }
    return false; // not default case
}

ExprRes EvalWhich::make_which_expr(Context& ctx) {
    auto res = Base::make_which_expr(ctx);
    if (codegen_enabled()) {
        auto& gen_ctx = gen().ctx_stack().top<gen::WhichContext>();
        exp::set_data(res, gen_ctx.tmp_var_name());
    }
    return res;
}

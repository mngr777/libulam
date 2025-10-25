#pragma once
#include "./env.hpp"
#include "./helper.hpp"
#include <libulam/sema/eval/which.hpp>

class EvalWhich : public ::EvalHelper, public ulam::sema::EvalWhich {
public:
    using Base = ulam::sema::EvalWhich;
    using ExprRes = ulam::sema::ExprRes;

    EvalWhich(EvalEnv& env): ::EvalHelper{env}, Base{env} {}

    void eval_which(ulam::Ref<ulam::ast::Which> node) override;

protected:
    ulam::Ptr<ulam::Var>
    make_which_var(Context& ctx, ulam::Ref<ulam::ast::Expr> expr) override;

    void eval_cases(Context& ctx) override;

    bool
    eval_case(Context& ctx, ulam::Ref<ulam::ast::WhichCase> case_) override;

    bool match_conds(
        Context& ctx,
        ulam::Ref<ulam::ast::WhichCaseCondList> case_conds) override;

    ExprRes match_expr_res(
        Context& ctx,
        ulam::Ref<ulam::ast::Expr> case_expr,
        ExprRes&& case_res) override;

    ExprRes make_which_expr(Context& ctx) override;
};

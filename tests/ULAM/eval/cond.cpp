#include "./cond.hpp"
#include "./expr_res.hpp"

using CondRes = EvalCond::CondRes;

CondRes EvalCond::eval_as_cond(ulam::Ref<ulam::ast::AsCond> as_cond) {
    auto cond_res = Base::eval_as_cond(as_cond);
    if (codegen_enabled())
        gen().add_as_cond(as_cond, cond_res.second.type());
    return cond_res;
}

CondRes EvalCond::eval_expr(ulam::Ref<ulam::ast::Expr> expr) {
    if (!codegen_enabled())
        return Base::eval_expr(expr);

    auto res = env().eval_expr(expr);
    res = env().to_boolean(expr, std::move(res));

    gen().append(exp::data(res));
    gen().append("cond");

    return {is_true(res), ulam::sema::AsCondContext{}};
}

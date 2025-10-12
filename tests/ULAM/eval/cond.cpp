#include "./cond.hpp"
#include "./expr_res.hpp"
#include "libulam/semantic/type/builtin_type_id.hpp"

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
    // t3374: no cast for Bool(n>1) -> Bool(1)?
    if (!res.type()->is(ulam::BoolId))
        res = env().to_boolean(expr, std::move(res));

    gen().append(exp::data(res));
    gen().append("cond");

    return {is_true(res), ulam::sema::AsCondContext{}};
}

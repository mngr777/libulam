#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

// EvalHelperBase

ExprRes EvalHelperBase::to_boolean(
    Scope* scope, Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags) {
    auto boolean = builtins().boolean();
    auto cast = eval().cast_helper(scope, flags);
    res = cast->cast(expr, boolean, std::move(res), false);
    if (!res || (!(flags & evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to cast to boolean value");
    return std::move(res);
}

// ScopedEvalHelper

ExprRes ScopedEvalHelper::to_boolean(
    Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags) {
    return to_boolean(scope(), expr, std::move(res), flags);
}

} // namespace ulam::sema

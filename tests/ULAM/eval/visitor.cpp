#include "./visitor.hpp"
#include "./cast.hpp"
#include "./expr_visitor.hpp"
#include "./funcall.hpp"
#include "./init.hpp"

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

ulam::sema::ExprRes EvalVisitor::eval_expr(ulam::Ref<ulam::ast::Expr> expr) {
    auto res = ulam::sema::EvalVisitor::eval_expr(expr);
    assert(res);
    debug() << "expr: "
            << (res.has_data() ? res.data<std::string>()
                               : std::string{"no data"})
            << "\n";
    return res;
}

ulam::Ptr<ulam::sema::EvalExprVisitor>
EvalVisitor::expr_visitor(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalExprVisitor>(*this, _stringifier, scope);
}

ulam::Ptr<ulam::sema::EvalInit>
EvalVisitor::init_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalInit>(*this, diag(), _program->str_pool(), scope);
}

ulam::Ptr<ulam::sema::EvalCast>
EvalVisitor::cast_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalCast>(*this, diag(), scope);
}

ulam::Ptr<ulam::sema::EvalFuncall>
EvalVisitor::funcall_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalFuncall>(*this, diag(), scope);
}

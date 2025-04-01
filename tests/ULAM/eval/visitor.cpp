#include "./visitor.hpp"
#include "./expr_visitor.hpp"
#include "./init.hpp"
#include "./cast.hpp"
#include "./funcall.hpp"

ulam::Ptr<ulam::sema::EvalExprVisitor>
EvalVisitor::expr_visitor(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalExprVisitor>(*this, scope);
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

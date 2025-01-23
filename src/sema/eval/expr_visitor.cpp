#include "libulam/sema/expr_visitor.hpp"
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/visitor.hpp>

namespace ulam::sema {
EvalExprVisitor::EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope):
    ExprVisitor{eval._program, scope}, _eval{eval} {}

ExprRes EvalExprVisitor::funcall(Ref<Fun> fun, TypedValueList&& args) {
    return _eval.funcall(fun, std::move(args));
}

}

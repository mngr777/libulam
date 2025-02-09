#include "libulam/sema/expr_visitor.hpp"
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/visitor.hpp>

#define DEBUG_EXPR_VISITOR // TEST
#ifdef DEBUG_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalExprVisitor] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {
EvalExprVisitor::EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope):
    ExprVisitor{eval._program, scope}, _eval{eval} {}

ExprRes EvalExprVisitor::funcall(Ref<Fun> fun, TypedValueList&& args) {
    debug() << __FUNCTION__ << "\n";
    return _eval.funcall(fun, std::move(args));
}

} // namespace ulam::sema

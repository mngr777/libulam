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

ExprRes EvalExprVisitor::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, LValue self, TypedValueList&& args) {
    debug() << __FUNCTION__ << "\n";
    if (fun->is_native()) {
        // can't eval, return empty value
        diag().notice(node, "cannot evaluate native function");
        if (fun->ret_type()->is_ref()) {
            LValue lval;
            lval.set_is_xvalue(false);
            return {fun->ret_type(), Value{lval}};
        }
        return {fun->ret_type(), Value{RValue{}}};

    } else if (fun->is_pure_virtual()) {
        diag().error(node, "function is pure virtual");
        return {ExprError::FunctionIsPureVirtual};
    }
    assert(fun->node()->has_body());
    return _eval.funcall(fun, self.self(), std::move(args));
}

} // namespace ulam::sema

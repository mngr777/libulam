#include "libulam/diag.hpp"
#include "libulam/semantic/type.hpp"
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/helper/array_dim_eval.hpp>

namespace ulam::sema {

std::pair<array_size_t, bool> ArrayDimEval::eval(Ref<ast::Expr> expr) {
    ExprVisitor ev{ast(), _scope};
    ExprRes res = expr->accept(ev);
    auto rval = res.value().rvalue();

    array_size_t size{0}; // TODO: what is max array size?
    if (rval->is_unknown()) {
        diag().emit(diag::Error, expr->loc_id(), 1, "cannot calculate");
        return {UnknownArraySize, false};

    } else if (rval->is<Unsigned>()) {
        size = rval->get<Unsigned>();

    } else if (rval->is<Integer>()) {
        if (rval->get<Integer>() < 0) {
            diag().emit(diag::Error, expr->loc_id(), 1, "negative value");
            return {UnknownArraySize, false};
        }
        size = (Unsigned)rval->get<Integer>();

    } else {
        diag().emit(diag::Error, expr->loc_id(), 1, "non-numeric value");
        return {UnknownArraySize, false};
    }

    return {size, true};
}

} // namespace ulam::sema

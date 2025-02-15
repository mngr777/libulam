#include <libulam/diag.hpp>
#include <libulam/sema/array_dim_eval.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

std::pair<array_size_t, bool> ArrayDimEval::eval(Ref<ast::Expr> expr) {
    ExprVisitor ev{_program, _scope};
    ExprRes res = expr->accept(ev);
    auto rval = res.move_value().move_rvalue();

    // !! TODO: this is wrong in general, e.g. doesn't distinguish Unsigned and
    // Unary

    array_size_t size{0}; // TODO: what is max array size?
    if (rval.empty()) {
        _program->diag().emit(
            Diag::Error, expr->loc_id(), 1, "cannot calculate");
        return {UnknownArraySize, false};

    } else if (rval.is<Unsigned>()) {
        size = rval.get<Unsigned>();

    } else if (rval.is<Integer>()) {
        if (rval.get<Integer>() < 0) {
            _program->diag().emit(
                Diag::Error, expr->loc_id(), 1, "negative value");
            return {UnknownArraySize, false};
        }
        size = (Unsigned)rval.get<Integer>();

    } else {
        _program->diag().emit(
            Diag::Error, expr->loc_id(), 1, "non-numeric value");
        return {UnknownArraySize, false};
    }

    return {size, true};
}

} // namespace ulam::sema

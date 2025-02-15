#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

std::pair<TypedValueList, bool>
ParamEval::eval(Ref<ast::ArgList> args, Ref<Scope> scope) {
    if (!args || args->child_num() == 0)
        return {TypedValueList{}, true};

    TypedValueList values;
    bool success = true;
    ExprVisitor ev{_program, scope};
    for (unsigned n = 0; n < args->child_num(); ++n) {
        auto arg = args->get(n);
        ExprRes res = arg->accept(ev);
        // has actual value?
        auto rval = res.move_value().move_rvalue();
        if (_flags & ReqValues && rval.empty()) {
            success = false;
            _program->diag().emit(
                Diag::Error, arg->loc_id(), 1, "failed to evaluate argument");
        }
        values.push_back(TypedValue{res.type(), std::move(rval)});
    }
    return {std::move(values), success};
}

} // namespace ulam::sema

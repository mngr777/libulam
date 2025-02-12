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
        const auto& value = res.value();
        // any result?
        if (value.is_nil())
            success = false;
        // has actual value?
        if (_flags & ReqValues && value.rvalue().empty()) {
            success = false;
            _program->diag().emit(
                Diag::Error, arg->loc_id(), 1, "failed to evaluate argument");
        }
        values.push_back(res.move_typed_value());
    }
    return {std::move(values), success};
}

} // namespace ulam::sema

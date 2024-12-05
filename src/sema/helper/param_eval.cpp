#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/helper/param_eval.hpp>

namespace ulam::sema {

std::pair<TypedValueList, bool> ParamEval::eval(ast::Ref<ast::ArgList> args, ScopeProxy scope) {
    TypedValueList values;
    assert(args->child_num() > 0);
    bool success = true;
    ExprVisitor ev{ast(), scope};
    for (unsigned n = 0; n < args->child_num(); ++n) {
        auto arg = args->get(n);
        ExprRes res = arg->accept(ev);
        const auto& value = res.value();
        // any result?
        if (value.is_nil())
            success = false;
        // has actual value?
        if (_flags & ReqValues && value.rvalue()->is_unknown()) {
            success = false;
            diag().emit(
                diag::Error, arg->loc_id(), 1, "failed to evaluate argument");
        }
        values.push_back(res.move_typed_value());
    }
    return {std::move(values), success};
}

} // namespace ulam::sema

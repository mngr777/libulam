#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/visitor.hpp>

namespace ulam::sema {

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, Ref<FunSet> fset, LValue self, TypedValueList&& args) {
    auto dyn_cls = self.dyn_cls();
    auto match_res = fset->find_match(dyn_cls, args);
    if (match_res.empty()) {
        _diag.error(node, "no matching functions found");
        return {ExprError::NoMatchingFunction};
    } else if (match_res.size() > 1) {
        _diag.error(node, "ambiguous function call");
        return {ExprError::AmbiguousFunctionCall};
    }

    auto cast = _eval.cast_helper(_scope);
    TypedValueList call_args{};
    auto fun = *match_res.begin();
    for (auto& param : fun->params()) {
        assert(!args.empty());

        auto arg = std::move(args.front());
        args.pop_front();

        auto param_type = param->type();

        // binding rvalue or xvalue lvalue to const ref
        if (param_type->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            param_type = param_type->deref(); // cast to non-ref type if casting
        }

        // cast to param type
        auto cast_res = cast->cast(node, param_type, std::move(arg), false);
        if (!cast_res)
            return cast_res;

        call_args.emplace_back(param_type, cast_res.move_value());
    }
    return funcall(node, fun, self, std::move(call_args));
}

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, LValue self, TypedValueList&& args) {
    if (fun->is_native()) {
        // can't eval, return empty value
        _diag.notice(node, "cannot evaluate native function");
        if (fun->ret_type()->is_ref()) {
            LValue lval;
            lval.set_is_xvalue(false);
            return {fun->ret_type(), Value{lval}};
        }
        return {fun->ret_type(), Value{RValue{}}};

    } else if (fun->is_pure_virtual()) {
        _diag.error(node, "function is pure virtual");
        return {ExprError::FunctionIsPureVirtual};
    }
    assert(fun->node()->has_body());
    return _eval.funcall(fun, self.self(), std::move(args));
}

} // namespace ulam::sema

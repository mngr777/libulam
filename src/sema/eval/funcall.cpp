#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>

namespace ulam::sema {

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args) {
    assert(callable);
    assert(args);
    assert(callable.type()->is(FunId));

    const auto& val = callable.value();
    assert(val.is_lvalue());
    assert(val.lvalue().is<BoundFunSet>());
    const auto& bound_fset = val.lvalue().get<BoundFunSet>();

    // find match
    auto fset = bound_fset.fset();
    auto dyn_cls = bound_fset.self().type()->as_class();
    auto [match_res, error] =
        find_match(node, fset, dyn_cls, args.typed_value_refs());
    if (error != ExprError::Ok)
        return {error};
    auto fun = *match_res.begin();

    // cast args
    args = cast_args(node, fun, std::move(args));

    return do_funcall(node, fun, std::move(callable), std::move(args));
}

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, Ref<FunSet> fset, LValue self, ExprResList&& args) {
    assert(args);

    auto [match_res, error] =
        find_match(node, fset, self.dyn_cls(), args.typed_value_refs());
    if (error != ExprError::Ok)
        return {error};
    auto fun = *match_res.begin();
    args = cast_args(node, fun, std::move(args));
    // TMP
    TypedValueList call_args;
    for (auto& arg : args)
        call_args.push_back(arg.move_typed_value());
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

ExprRes EvalFuncall::do_funcall(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& callable, ExprResList&& args) {
    // TMP
    TypedValueList call_args;
    for (auto& arg : args)
        call_args.push_back(arg.move_typed_value());
    auto val = callable.move_value();
    return funcall(node, fun, val.self(), std::move(call_args));
}

std::pair<FunSet::Matches, ExprError> EvalFuncall::find_match(
    Ref<ast::Node> node,
    Ref<FunSet> fset,
    Ref<Class> dyn_cls,
    const TypedValueRefList& args) {

    auto error = ExprError::Ok;
    auto match_res = fset->find_match(dyn_cls, args);
    if (match_res.empty()) {
        _diag.error(node, "no matching functions found");
        error = ExprError::NoMatchingFunction;
    } else if (match_res.size() > 1) {
        _diag.error(node, "ambiguous function call");
        error = ExprError::AmbiguousFunctionCall;
    }
    return {std::move(match_res), error};
}

ExprResList
EvalFuncall::cast_args(Ref<ast::Node> node, Ref<Fun> fun, ExprResList&& args) {
    assert(fun->params().size() == args.size());
    auto cast = _eval.cast_helper(_scope);

    auto arg_it = args.begin();
    auto param_it = fun->params().begin();
    for (; arg_it != args.end(); ++arg_it, ++param_it) {
        auto& arg = *arg_it;
        auto& param = *param_it;
        auto param_type = param->type();

        // binding rvalue or xvalue lvalue to const ref
        if (param_type->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            param_type = param_type->deref(); // cast to non-ref type if casting
        }

        arg = cast->cast(node, param_type, std::move(arg));
        assert(arg);
    }
    return std::move(args);
}

} // namespace ulam::sema

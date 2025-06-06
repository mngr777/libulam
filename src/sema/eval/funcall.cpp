#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalFuncall] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

ExprRes EvalFuncall::construct(
    Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args) {
    if (!cls->has_constructors())
        return {ExprError::NoMatchingFunction};

    auto rval = cls->construct();
    auto [match_res, error] =
        find_match(node, cls->constructors(), cls, args.typed_value_refs());
    if (error != ExprError::Ok)
        return {error};
    auto fun = *match_res.begin();

    args = cast_args(node, fun, std::move(args));
    return construct_funcall(node, cls, fun, std::move(rval), std::move(args));
}

ExprRes EvalFuncall::construct_funcall(
    Ref<ast::Node> node,
    Ref<Class> cls,
    Ref<Fun> fun,
    RValue&& rval,
    ExprResList&& args) {
    rval.set_is_consteval(args.is_consteval());
    do_funcall(node, fun, rval.self(), std::move(args));
    return {cls, Value{std::move(rval)}};
}

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args) {
    assert(callable);
    assert(args);
    assert(callable.type()->is(FunId));

    if (flags() & evl::Consteval)
        return {ExprError::NotConsteval};

    const auto& val = callable.value();
    assert(val.is_lvalue());
    assert(val.lvalue().is<BoundFunSet>());
    const auto& bfset = val.lvalue().get<BoundFunSet>();

    auto fset = bfset.fset();
    auto [match_res, error] =
        find_match(node, fset, bfset.dyn_cls(), args.typed_value_refs());
    if (error != ExprError::Ok)
        return {error};
    auto fun = *match_res.begin();

    args = cast_args(node, fun, std::move(args));
    return funcall_callable(node, fun, std::move(callable), std::move(args));
}

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    assert(args);

    if (flags() & evl::Consteval)
        return {ExprError::NotConsteval};

    args = cast_args(node, fun, std::move(args));
    return funcall_obj(node, fun, std::move(obj), std::move(args));
}

ExprRes EvalFuncall::funcall_callable(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& callable, ExprResList&& args) {
    assert(callable.type()->is(FunId));
    auto val = callable.move_value();
    assert(!val.empty());
    return do_funcall(node, fun, val.self(), std::move(args));
}

ExprRes EvalFuncall::funcall_obj(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    assert(obj.type()->actual()->is_class());
    auto val = obj.move_value();
    auto self = val.empty() ? LValue{} : val.self();
    return do_funcall(node, fun, self, std::move(args));
}

ExprRes EvalFuncall::do_funcall(
    Ref<ast::Node> node, Ref<Fun> fun, LValue self, ExprResList&& args) {
    if (has_flag(evl::NoExec))
        return empty_ret_val(node, fun);

    if (fun->is_native()) {
        // can't eval, return empty value
        diag().notice(node, "cannot evaluate native function");
        return empty_ret_val(node, fun);

    } else if (fun->is_pure_virtual()) {
        diag().error(node, "function is pure virtual");
        return {ExprError::FunctionIsPureVirtual};
    }

    assert(fun->node()->has_body());
    assert(!self.empty());
    return eval().funcall(fun, self, std::move(args));
}

ExprRes EvalFuncall::empty_ret_val(Ref<ast::Node> node, Ref<Fun> fun) {
    if (fun->ret_type()->is_ref()) {
        LValue lval;
        lval.set_is_xvalue(false);
        return {fun->ret_type(), Value{lval}};
    }
    return {fun->ret_type(), Value{RValue{}}};
}

std::pair<FunSet::Matches, ExprError> EvalFuncall::find_match(
    Ref<ast::Node> node,
    Ref<FunSet> fset,
    Ref<Class> dyn_cls,
    const TypedValueRefList& args) {

    auto error = ExprError::Ok;
    auto match_res = fset->find_match(dyn_cls, args);
    if (match_res.empty()) {
        diag().error(node, "no matching functions found");
        error = ExprError::NoMatchingFunction;
    } else if (match_res.size() > 1) {
        diag().error(node, "ambiguous function call");
        error = ExprError::AmbiguousFunctionCall;
    }
    return {std::move(match_res), error};
}

ExprResList
EvalFuncall::cast_args(Ref<ast::Node> node, Ref<Fun> fun, ExprResList&& args) {
    assert(fun->has_ellipsis() || fun->params().size() == args.size());
    assert(!fun->has_ellipsis() || fun->params().size() <= args.size());

    auto arg_it = args.begin();
    auto param_it = fun->params().begin();
    for (; arg_it != args.end(); ++arg_it, ++param_it) {
        if (param_it == fun->params().end()) {
            assert(fun->has_ellipsis());
            break;
        }
        auto& arg = *arg_it;
        auto& param = *param_it;
        auto param_type = param->type();

        // binding rvalue or xvalue lvalue to const ref
        if (param_type->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            param_type = param_type->deref(); // cast to non-ref type if casting
        }

        arg = cast_arg(node, fun, ref(param), param_type, std::move(arg));
        assert(arg);
    }
    return std::move(args);
}

ExprRes EvalFuncall::cast_arg(
    Ref<ast::Node> node,
    Ref<Fun> fun,
    Ref<Var> param,
    Ref<Type> to,
    ExprRes&& arg) {
    auto cast = eval().cast_helper(scope(), flags());
    return cast->cast(node, to, std::move(arg));
}

} // namespace ulam::sema

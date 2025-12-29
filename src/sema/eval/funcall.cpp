#include "libulam/semantic/utils/strf.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
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

ExprRes
EvalFuncall::call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args) {
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
    return funcall_callable(
        node, fun, std::move(callable), std::move(args), bfset.eff_cls());
}

ExprRes EvalFuncall::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    assert(args);

    if (flags() & evl::Consteval)
        return {ExprError::NotConsteval};

    args = cast_args(node, fun, std::move(args));
    return funcall_obj(node, fun, std::move(obj), std::move(args));
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

ExprRes EvalFuncall::funcall_callable(
    Ref<ast::Node> node,
    Ref<Fun> fun,
    ExprRes&& callable,
    ExprResList&& args,
    Ref<Class> eff_cls) {
    assert(callable.type()->is(FunId));
    if (has_flag(evl::NoExec))
        return empty_ret_val(node, fun);
    auto val = callable.move_value();
    assert(!val.empty());
    return do_funcall(node, fun, val.self(), std::move(args), eff_cls);
}

ExprRes EvalFuncall::funcall_obj(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    assert(obj.type()->actual()->is_class());
    if (has_flag(evl::NoExec))
        return empty_ret_val(node, fun);
    auto val = obj.move_value();
    auto self = val.empty() ? LValue{} : val.self();
    return do_funcall(node, fun, self, std::move(args));
}

ExprRes EvalFuncall::do_funcall(
    Ref<ast::Node> node,
    Ref<Fun> fun,
    LValue self,
    ExprResList&& args,
    Ref<Class> eff_cls) {

    if (fun->is_native()) {
        return do_funcall_native(node, fun, self, std::move(args));
    }
    if (fun->is_pure_virtual()) {
        diag().error(node, "function is pure virtual");
        return {ExprError::FunctionIsPureVirtual};
    }
    assert(fun->node()->has_body());
    assert(!self.empty());

    if (self.has_auto_scope_lvl())
        self.set_scope_lvl(env().scope_lvl() + 1);
    auto stack_raii = env().stack_raii(fun, self);
    auto sr = env().fun_scope_raii(fun, self, eff_cls);

    // bind params
    std::list<Ptr<ast::VarDef>> tmp_defs{};
    std::list<Ptr<Var>> tmp_vars{};
    for (const auto& param : fun->params()) {
        assert(!args.empty());
        auto arg = args.pop_front();

        // binding rvalue or xvalue lvalue to const ref via tmp variable
        if (param->type()->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            // create tmp var default
            auto def = make<ast::VarDef>(param->node()->name());
            auto var = make<Var>(
                param->type_node(), ref(def),
                TypedValue{param->type()->deref(), arg.move_value().deref()},
                param->flags() | Var::TmpFunParam);
            arg = {param->type(), Value{LValue{ref(var)}}};
            tmp_defs.push_back(std::move(def));
            tmp_vars.push_back(std::move(var));
        }
        assert(arg.type()->is_same(param->type()));

        auto var = make<Var>(
            param->type_node(), param->node(), param->type(), param->flags());
        var->set_value(arg.move_value());
        scope()->set(var->name_id(), std::move(var));
    }

    // eval
    try {
        env().eval_stmt(fun->body_node());
    } catch (EvalExceptReturn& ret) {
        debug() << "}\n";
#ifdef ULAM_DEBUG
        utils::Strf strf{program()};
        debug() << "retval: " << strf.str(fun->ret_type(), ret.res().value())
                << "\n";
#endif
        return ret.move_res();
    }
    debug() << "}\n";

    auto ret_type = fun->ret_type();
    if (!ret_type->is(VoidId)) {
        if (has_flag(evl::NoExec)) {
            if (ret_type->is_ref()) {
                LValue lval;
                lval.set_is_xvalue(false);
                return {ret_type, Value{lval}};
            }
            return {ret_type, Value{ret_type->construct_ph()}};
        } else {
            return {ExprError::NoReturn};
        }
    }
    return {builtins().void_type(), Value{RValue{}}};
}

ExprRes EvalFuncall::do_funcall_native(
    Ref<ast::Node> node, Ref<Fun> fun, LValue self, ExprResList&& args) {
    // can't eval, return empty value
    diag().notice(node, "cannot evaluate native function");
    return empty_ret_val(node, fun);
}

ExprRes EvalFuncall::empty_ret_val(Ref<ast::Node> node, Ref<Fun> fun) {
    if (!fun->ret_type()->is_ref())
        return {fun->ret_type(), Value{RValue{}}};

    LValue lval;
    lval.set_is_xvalue(false);
    return {fun->ret_type(), Value{lval}};
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
    return env().cast(node, to, std::move(arg));
}

} // namespace ulam::sema

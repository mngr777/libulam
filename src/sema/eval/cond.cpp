#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

CondRes EvalCond::eval_cond(Ref<ast::Cond> cond) {
    return cond->is_as_cond() ? eval_as_cond(cond->as_cond())
                              : eval_expr(cond->expr());
}

CondRes EvalCond::eval_as_cond(Ref<ast::AsCond> as_cond) {
    auto res = eval_as_cond_ident(as_cond->ident());
    auto type = resolve_as_cond_type(as_cond->type_name());
    assert(!type->is_ref());

    bool is_match = false;
    if (!res.value().empty()) {
        auto dyn_type = res.value().dyn_obj_type(true);
        is_match = dyn_type->is_impl_refable_as(type, res.value());
    }
    if (!is_match && !has_flag(evl::NoExec)) // TODO
        return {is_match, AsCondContext{type}};
    auto [var_def, var] = define_as_cond_var(as_cond, std::move(res), type);
    return {is_match, AsCondContext{type, std::move(var_def), var}};
}

CondRes EvalCond::eval_expr(Ref<ast::Expr> expr) {
    auto res = env().eval_expr(expr);
    res = env().to_boolean(expr, std::move(res));
    return {is_true(res), AsCondContext{}};
}

ExprRes EvalCond::eval_as_cond_ident(Ref<ast::Ident> ident) {
    auto res = env().eval_expr(ident);
    auto arg_type = res.type()->actual();
    if (!arg_type->is_object()) {
        diag().error(ident, "not a class or Atom");
        throw EvalExceptError("if-as var is not an object");
    }
    return res;
}

Ref<Type> EvalCond::resolve_as_cond_type(Ref<ast::TypeName> type_name) {
    auto type = env().resolver(false).resolve_type_name(type_name, scope());
    if (!type)
        throw EvalExceptError("failed to resolve type");
    if (!type->is_object()) {
        diag().error(type_name, "not a class or Atom");
        throw EvalExceptError("if-as type is not class or Atom");
    }
    return type;
}

EvalCond::VarDefPair EvalCond::define_as_cond_var(
    Ref<ast::AsCond> node, ExprRes&& res, Ref<Type> type) {

    Ptr<ast::VarDef> def{};
    Ref<Var> ref{};

    LValue lval;
    if (!res.value().empty()) {
        assert(res.value().is_lvalue());
        lval = res.move_value().lvalue().as(type);
    } else {
        if (!has_flag(evl::NoExec)) {
            diag().error(node, "empty value");
            throw EvalExceptError("empty value");
        }
    }

    if (node->ident()->is_self()) {
        scope()->ctx().set_self(lval, type->as_class());
    } else {
        def = make<ast::VarDef>(node->ident()->name());
        auto var = make<Var>(
            node->type_name(), ulam::ref(def),
            TypedValue{type->ref_type(), Value{lval}});
        var->set_scope_lvl(env().stack_size());
        ref = ulam::ref(var);
        scope()->set(var->name_id(), std::move(var));
    }

    return {std::move(def), ref};
}

} // namespace ulam::sema

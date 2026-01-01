#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/flags.hpp>
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
    if (res.value().has_rvalue()) {
        auto dyn_type = res.value().dyn_obj_type(true);
        is_match = dyn_type->is_impl_refable_as(type, res.value());
    }
    if (!is_match && !has_flag(evl::NoExec))
        return {is_match, AsCondContext{type}};

    if (as_cond->ident()->is_self()) {
        AsCondContext as_cond_ctx{
            type, as_cond_lvalue(as_cond, std::move(res), type)};
        return {is_match, std::move(as_cond_ctx)};
    }

    auto [var_def, var] = make_as_cond_var(as_cond, std::move(res), type);
    return {is_match, AsCondContext{type, std::move(var_def), std::move(var)}};
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
    // TODO: can this actually be Atom?
    if (!type->is_object()) {
        diag().error(type_name, "not a class or Atom");
        throw EvalExceptError("if-as type is not class or Atom");
    }
    return type;
}

LValue
EvalCond::as_cond_lvalue(Ref<ast::AsCond> node, ExprRes&& res, Ref<Type> type) {
    LValue lval;
    if (res.value().has_rvalue()) {
        assert(res.value().is_lvalue());
        lval = res.move_value().lvalue().as(type);
    } else {
        if (!has_flag(evl::NoExec)) {
            diag().error(node, "empty value");
            throw EvalExceptError("empty value");
        }
    }
    return lval;
}

EvalCond::VarDefPair EvalCond::make_as_cond_var(
    Ref<ast::AsCond> node, ExprRes&& res, Ref<Type> type) {
    assert(!node->ident()->is_self());

    auto lval = as_cond_lvalue(node, std::move(res), type);
    auto def = make<ast::VarDef>(node->ident()->name());
    auto var = make<Var>(
        node->type_name(), ulam::ref(def),
        TypedValue{type->ref_type(), Value{lval}});
    return {std::move(def), std::move(var)};
}

} // namespace ulam::sema

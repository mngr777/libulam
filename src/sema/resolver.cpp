#include "libulam/semantic/scope.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/helper/array_dim_eval.hpp>
#include <libulam/sema/helper/param_eval.hpp>
#include <libulam/sema/resolver.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::Resolver] "
#include "src/debug.hpp"

#define CHECK_STATE(obj)                                                       \
    do {                                                                       \
        auto is_resolved = check_state(obj);                                   \
        if (is_resolved.has_value())                                           \
            return is_resolved.value();                                        \
    } while (false)

#define RET_UPD_STATE(obj, is_resolved)                                        \
    do {                                                                       \
        update_state((obj), (is_resolved));                                    \
        return (is_resolved);                                                  \
    } while (false)

namespace ulam::sema {

void Resolver::resolve() {
    for (auto& module : _program->modules())
        resolve(ref(module));
    for (auto cls : _classes)
        resolve(cls);
    _classes = {};
}

void Resolver::resolve(Ref<Module> module) {
    for (auto& pair : *module) {
        auto& [name_id, sym] = pair;
        if (sym.is<Class>()) {
            init(sym.get<Class>());
        } else {
            assert(sym.is<ClassTpl>());
            resolve(sym.get<ClassTpl>());
        }
    }
}

bool Resolver::resolve(Ref<ClassTpl> cls_tpl) {
    CHECK_STATE(cls_tpl);
    bool is_resolved = true;

    // params
    {
        auto scope_proxy = cls_tpl->param_scope()->proxy(0);
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (name_id == NoStrId)
                break;
            assert(sym && sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is(Var::Const | Var::ClassParam | Var::Tpl));
            is_resolved = resolve(var, &scope_proxy) && is_resolved;
        }
    }

    RET_UPD_STATE(cls_tpl, is_resolved);
}

bool Resolver::init(Ref<Class> cls) {
    bool success = true;

    // params
    {
        auto scope_proxy = cls->param_scope()->proxy(0);
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (!sym)
                break;
            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is(Var::Const | Var::ClassParam));
            success = resolve(var, &scope_proxy) && success;
        }
    }

    // init inherited members
    {
        if (cls->node()->has_ancestors()) {
            auto ancestors = cls->node()->ancestors();
            auto scope = cls->inh_scope();
            for (unsigned n = 0; n < ancestors->child_num(); ++n) {
                // resolve ancestor type
                auto type_name = ancestors->get(n);
                auto type = resolve_type_name(type_name, scope);
                if (!type->is_class()) {
                    diag().emit(
                        diag::Error, type_name->loc_id(), 1, "not a class");
                    success = false;
                    continue;
                }
                auto anc_cls = type->as_class();
                // add ancestor class
                cls->add_ancestor(anc_cls, type_name);
                // import symbols
                anc_cls->members().export_symbols(
                    cls->members(), false /* do not overwrite */);
            }
        }
    }

    if (!success)
        RET_UPD_STATE(cls, false);

    _classes.push_back(cls);
    return true;
}

bool Resolver::resolve(Ref<Class> cls) {
    CHECK_STATE(cls);
    bool is_resolved = true;

    // resolve ancestors
    for (auto anc : cls->ancestors()) {
        if (!resolve(anc.cls())) {
            diag().emit(
                diag::Error, anc.node()->loc_id(), 1,
                "cannot resolve ancestor type");
            is_resolved = false;
        }
    }

    // members
    {
        auto scope_proxy = cls->scope()->proxy();
        scope_proxy.reset();
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (!sym)
                break;
            bool res{};
            if (sym->is<UserType>()) {
                // alias
                auto type = sym->get<UserType>();
                assert(type->is_alias());
                res = resolve(type->as_alias(), &scope_proxy);
            } else if (sym->is<Var>()) {
                // var
                res = resolve(sym->get<Var>(), &scope_proxy);
            } else {
                // fun
                assert(sym->is<Fun>());
                res = resolve(cls, sym->get<Fun>());
            }
            is_resolved = is_resolved && res;
        }
    }

    RET_UPD_STATE(cls, is_resolved);
}

bool Resolver::resolve(Ref<AliasType> alias, Ref<Scope> scope) {
    CHECK_STATE(alias);
    auto type_name = alias->node()->type_name();
    auto type_expr = alias->node()->type_expr();

    // type
    auto type = resolve_type_name(type_name, scope);
    if (!type)
        RET_UPD_STATE(alias, false);

    // []
    if (type_expr->has_array_dims())
        type = apply_array_dims(type, type_expr->array_dims(), scope);
    if (!type)
        RET_UPD_STATE(alias, false);

    // &
    if (type_expr->is_ref())
        type = type->ref_type();

    alias->set_aliased(type);
    RET_UPD_STATE(alias, true);
}

bool Resolver::resolve(Ref<Var> var, Ref<Scope> scope) {
    CHECK_STATE(var);
    bool is_resolved = true;
    auto node = var->node();
    auto type_name = var->type_node();

    // type
    if (!var->type()) {
        auto type = resolve_var_decl_type(type_name, node, scope);
        if (type)
            var->set_type(type);
        is_resolved = type && is_resolved;
    }

    // value
    if (var->is_const() && !var->value()) {
        if (node->has_default_value()) {
            ExprVisitor ev{ast(), scope};
            ExprRes res = node->default_value()->accept(ev);
            auto tv = res.move_typed_value();
            // TODO: conversion/type error, check if const
            var->value() = tv.move_value();
            if (var->value().is_nil()) {
                auto name = node->name();
                diag().emit(
                    diag::Error, name.loc_id(), str(name.str_id()).size(),
                    "cannot calculate constant value");
                is_resolved = false;
            }
        } else if (var->requires_value()) {
            auto name = node->name();
            diag().emit(
                diag::Error, name.loc_id(), str(name.str_id()).size(),
                "constant value required");
            is_resolved = false;
        }
    }

    RET_UPD_STATE(var, is_resolved);
}

bool Resolver::resolve(Ref<Class> cls, Ref<Fun> fun) {
    CHECK_STATE(fun);
    bool is_resolved = true;

    auto scope = cls->scope();
    for (auto& overload : fun->overloads()) {
        auto overload_ref = overload.ref();
        auto scope_proxy = scope->proxy(overload_ref->scope_version());
        is_resolved = resolve(overload_ref, &scope_proxy) && is_resolved;
    }

    RET_UPD_STATE(fun, is_resolved);
}

bool Resolver::resolve(Ref<FunOverload> overload, Ref<Scope> scope) {
    CHECK_STATE(overload);
    bool is_resolved = true;

    // return type
    auto ret_type_node = overload->ret_type_node();
    auto ret_type = resolve_fun_ret_type(ret_type_node, scope);
    if (!ret_type) {
        diag().emit(
            diag::Error, ret_type_node->loc_id(), 1,
            "cannot resolve return type");
        is_resolved = false;
    }

    // params
    BasicScope param_scope{scope}; // tmp scope
    ExprVisitor ev{ast(), &param_scope};
    auto params_node = overload->params_node();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        auto param_type = resolve_var_decl_type(
            param_node->type_name(), param_node, &param_scope);
        Value default_value{};
        if (param_type) {
            if (param_node->has_default_value()) {
                ExprRes res = param_node->default_value()->accept(ev);
                auto tv = res.move_typed_value();
                // TODO: conversion/type error
                default_value = tv.move_value();
            }
        }
        overload->add_param(param_type, std::move(default_value));
        is_resolved = param_type && is_resolved;
    }

    RET_UPD_STATE(overload, is_resolved);
}

Ref<Type> Resolver::resolve_var_decl_type(
    Ref<ast::TypeName> type_name, Ref<ast::VarDecl> node, Ref<Scope> scope) {
    // base type
    auto type = resolve_type_name(type_name, scope);
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(type, node->array_dims(), scope);

    // &
    if (node->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type>
Resolver::resolve_fun_ret_type(Ref<ast::FunRetType> node, Ref<Scope> scope) {
    // base type
    auto type = resolve_type_name(node->type_name(), scope);
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(type, node->array_dims(), scope);

    // &
    if (node->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type>
Resolver::resolve_type_name(Ref<ast::TypeName> type_name, Ref<Scope> scope) {

    // ast::TypeSpec to type
    auto type_spec = type_name->first();
    Ref<Type> type{};
    if (type_spec->type()) {
        // already has type
        type = type_spec->type();

    } else if (type_spec->type_tpl()) {
        // non-class tpl
        // TODO: pass resolver
        ParamEval pe{_program->ast()};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = type_spec->type_tpl()->type(
            diag(), type_spec->args(), std::move(args));

    } else if (type_spec->cls_tpl()) {
        // class tpl
        if (!resolve(type_spec->cls_tpl()))
            return {};
        // TODO: pass resolver
        ParamEval pe{_program->ast()};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = type_spec->cls_tpl()->type(
            diag(), type_spec->args(), std::move(args));
        if (type) {
            assert(type->is_class());
            init(type->as_class());
        }
    }
    if (!type)
        return {};

    // builtin?
    if (type_spec->is_builtin()) {
        // builtins don't have members (??)
        // TODO: write a sensible error message, also catch this in parser?
        if (type_name->child_num() > 1) {
            auto ident = type_name->ident(1);
            auto name_id = ident->name().str_id();
            diag().emit(
                diag::Error, ident->loc_id(), str(name_id).size(),
                "built-ins don't have member types");
        }
        return type;
    }

    // follow rest of type idents, resolve aliases along the way
    unsigned n = 0;
    while (true) {
        auto ident = type_name->ident(n);
        auto name_id = ident->name().str_id();
        // recursively resolve aliases
        if (type->is_alias()) {
            if (!resolve(type->as_alias(), {})) {
                diag().emit(
                    diag::Error, ident->loc_id(), str(name_id).size(),
                    "cannot resolve");
                return {};
            }
        }
        // done?
        if (++n == type_name->child_num()) {
            if (type->canon()->is_class()) {
                // class type itself is required, resolve it now
                if (!resolve(type->canon()->as_class())) {
                    diag().emit(
                        diag::Error, ident->loc_id(), str(name_id).size(),
                        "cannot resolve");
                    return {};
                }
            }
            return type;
        }

        // in A(x).B.C, A(x) and A(x).B must resolve to classes
        assert(type->canon());
        auto canon = type->canon();
        if (!canon->is_class()) {
            diag().emit(
                diag::Error, ident->loc_id(), str(name_id).size(),
                "not a class");
            return {};
        }
        auto cls = canon->as_class();

        // move to class member
        // can it be same class? e.g.
        // `quark A { typedef Int B; typedef A.B C; }`
        ident = type_name->ident(n);
        name_id = ident->name().str_id();
        if (!cls->get(name_id)) {
            diag().emit(
                diag::Error, ident->loc_id(), str(name_id).size(),
                "name not found in class");
            return {};
        }
        auto sym = cls->get(name_id);
        assert(sym->is<UserType>());
        type = sym->get<UserType>();
        // ok so far, loop back to alias resolution
    }
}

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type, Ref<ast::ExprList> dims, Ref<Scope> scope) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    ArrayDimEval eval{ast(), scope};
    for (unsigned n = 0; n < dims->child_num(); ++n) {
        auto expr = dims->get(n);
        auto [size, success] = eval.eval(expr);
        if (!success)
            return {};
        type = type->array_type(size);
    }
    return type;
}

std::optional<bool> Resolver::check_state(Ref<ScopeObject> obj) {
    if (obj->res_state() == ScopeObject::Resolved)
        return true;
    if (obj->res_state() == ScopeObject::Resolving)
        obj->set_res_state(ScopeObject::Unresolvable);
    if (obj->res_state() == ScopeObject::Unresolvable)
        return false;
    obj->set_res_state(ScopeObject::Resolving);
    return {};
}

void Resolver::update_state(Ref<ScopeObject> obj, bool is_resolved) {
    obj->set_res_state(
        is_resolved ? ScopeObject::Resolved : ScopeObject::Unresolvable);
}

std::string_view Resolver::str(str_id_t str_id) const {
    return _program->ast()->ctx().str(str_id);
}

} // namespace ulam::sema

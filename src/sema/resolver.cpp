#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/array_dim_eval.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/scope/view.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::Resolver] "
#include "src/debug.hpp"

#define CHECK_STATE(decl)                                                      \
    do {                                                                       \
        auto is_resolved = check_state(decl);                                  \
        if (is_resolved.has_value())                                           \
            return is_resolved.value();                                        \
    } while (false)

#define RET_UPD_STATE(decl, is_resolved)                                       \
    do {                                                                       \
        update_state((decl), (is_resolved));                                   \
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
        auto scope_view = cls_tpl->param_scope()->view(0);
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (name_id == NoStrId)
                break;
            assert(sym && sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is(Var::Const | Var::ClassParam | Var::Tpl));
            is_resolved = resolve(var, ref(scope_view)) && is_resolved;
        }
    }

    RET_UPD_STATE(cls_tpl, is_resolved);
}

bool Resolver::init(Ref<Class> cls) {
    if (cls->state() != Decl::NotResolved)
        return cls->is_ready();

    bool success = true;

    // params
    {
        auto scope_view = cls->param_scope()->view(0);
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (!sym)
                break;
            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is(Var::Const | Var::ClassParam));
            success = resolve(var, ref(scope_view)) && success;
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
                        Diag::Error, type_name->loc_id(), 1, "not a class");
                    success = false;
                    continue;
                }
                // add ancestor class
                cls->add_ancestor(type->as_class(), type_name);
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
    for (auto anc : cls->parents()) {
        if (!resolve(anc->cls())) {
            diag().emit(
                Diag::Error, anc->node()->loc_id(), 1,
                "cannot resolve ancestor type");
            is_resolved = false;
        }
    }

    // members
    {
        auto scope_view = cls->scope()->view();
        scope_view->reset();
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (!sym)
                break;
            bool res{};
            if (sym->is<UserType>()) {
                // alias
                auto type = sym->get<UserType>();
                assert(type->is_alias());
                res = resolve(type->as_alias(), ref(scope_view));
            } else if (sym->is<Var>()) {
                // var
                res = resolve(sym->get<Var>(), ref(scope_view));
            } else {
                // fun
                assert(sym->is<FunSet>());
                res = resolve(cls, sym->get<FunSet>());
            }
            is_resolved = is_resolved && res;
        }
    }

    // add funs from ancestors
    for (auto anc : cls->parents()) {
        for (auto& pair : anc->cls()->members()) {
            auto& [name_id, sym] = pair;
            if (!sym.is<FunSet>())
                continue;

            if (cls->has(name_id)) {
                // symbol found in class
                auto cls_sym = cls->get(name_id);
                // is it a var?
                if (cls_sym->is<Var>()) {
                    auto var = cls_sym->get<Var>();
                    diag().emit(
                        Diag::Notice, var->node()->loc_id(),
                        str(var->name_id()).size(),
                        "variable shadows inherited function");
                    continue;
                }
                //  must be a fun set, merge
                assert(cls_sym->is<FunSet>());
                cls_sym->get<FunSet>()->merge(sym.get<FunSet>());

            } else {
                // add parent fun set
                cls->set(name_id, sym.get<FunSet>());
            }
        }
    }

    RET_UPD_STATE(cls, is_resolved);
}

bool Resolver::resolve(Ref<AliasType> alias, Ref<Scope> scope) {
    assert(scope);
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
    assert(scope);
    CHECK_STATE(var);
    bool is_resolved = true;
    auto node = var->node();
    auto type_name = var->type_node();

    // type
    if (!var->has_type()) {
        auto type = resolve_var_decl_type(type_name, node, scope);
        if (type)
            var->set_type(type);
        is_resolved = type && is_resolved;
    }

    // value
    if (var->is_const() && !var->value()) {
        if (node->has_default_value()) {
            ExprVisitor ev{_program, scope};
            ExprRes res = node->default_value()->accept(ev);
            // impl. cast to var type
            if (res.type() && var->type() != res.type()) {
                res = ev.cast(node->assign_loc_id(), 1, std::move(res), var->type(), false);
                RET_UPD_STATE(var, false);
            }
            auto tv = res.move_typed_value();
            // TODO: conversion/type error, check if const
            var->value() = tv.move_value();
            if (var->value().is_nil()) {
                auto name = node->name();
                diag().emit(
                    Diag::Error, name.loc_id(), str(name.str_id()).size(),
                    "cannot calculate constant value");
                is_resolved = false;
            }
        } else if (var->requires_value()) {
            auto name = node->name();
            diag().emit(
                Diag::Error, name.loc_id(), str(name.str_id()).size(),
                "constant value required");
            is_resolved = false;
        }
    }

    RET_UPD_STATE(var, is_resolved);
}

bool Resolver::resolve(Ref<Class> cls, Ref<FunSet> fset) {
    CHECK_STATE(fset);
    bool is_resolved = true;

    auto scope = cls->scope();
    fset->for_each([&](Ref<Fun> fun) {
        auto scope_view = scope->view(fun->scope_version());
        is_resolved = resolve(fun, ref(scope_view)) && is_resolved;
    });
    fset->init_map(diag(), _program->str_pool());

    RET_UPD_STATE(fset, is_resolved);
}

bool Resolver::resolve(Ref<Fun> fun, Ref<Scope> scope) {
    assert(scope);
    CHECK_STATE(fun);
    bool is_resolved = true;

    // return type
    auto ret_type_node = fun->ret_type_node();
    auto ret_type = resolve_fun_ret_type(ret_type_node, scope);
    if (!ret_type) {
        diag().emit(
            Diag::Error, ret_type_node->loc_id(), 1,
            "cannot resolve return type");
        is_resolved = false;
    }

    // params
    BasicScope param_scope{scope}; // tmp scope
    ExprVisitor ev{_program, &param_scope};
    auto params_node = fun->params_node();
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
        fun->add_param(param_type, std::move(default_value));
        is_resolved = param_type && is_resolved;
    }

    RET_UPD_STATE(fun, is_resolved);
}

Ref<Type> Resolver::resolve_var_decl_type(
    Ref<ast::TypeName> type_name, Ref<ast::VarDecl> node, Ref<Scope> scope) {
    assert(scope);

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
    assert(scope);

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
    assert(scope);

    auto type_spec = type_name->first();
    auto type = resolve_type_spec(type_spec, scope);
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
                Diag::Error, ident->loc_id(), str(name_id).size(),
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
            if (!resolve(type->as_alias(), scope)) {
                diag().emit(
                    Diag::Error, ident->loc_id(), str(name_id).size(),
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
                        Diag::Error, ident->loc_id(), str(name_id).size(),
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
                Diag::Error, ident->loc_id(), str(name_id).size(),
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
                Diag::Error, ident->loc_id(), str(name_id).size(),
                "name not found in class");
            return {};
        }
        auto sym = cls->get(name_id);
        assert(sym->is<UserType>());
        type = sym->get<UserType>();
        // ok so far, loop back to alias resolution
    }
}

Ref<Type>
Resolver::resolve_type_spec(Ref<ast::TypeSpec> type_spec, Ref<Scope> scope) {
    if (type_spec->type())
        return type_spec->type();

    // builtin type tpl?
    if (type_spec->type_tpl()) {
        // non-class tpl
        ParamEval pe{_program};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        auto type = type_spec->type_tpl()->type(
            diag(), type_spec->args(), std::move(args));
        type_spec->set_type(type);
        return type;
    }

    // class tpl?
    if (type_spec->cls_tpl()) {
        // class tpl
        if (!resolve(type_spec->cls_tpl()))
            return {};
        ParamEval pe{_program};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        auto type = type_spec->cls_tpl()->type(
            diag(), type_spec->args(), std::move(args));
        if (!type)
            return {};
        assert(type->is_class());
        if (!init(type->as_class()))
            return {};
        return type;
    }

    // try searching for local alias
    if (scope->in(scp::Fun)) {
        auto name_id = type_spec->ident()->name().str_id();
        auto sym = scope->get(name_id);
        if (!sym)
            return {};
        auto type = sym->get<UserType>();
        assert(type->is_alias());
        return type;
    }

    return {};
}

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type, Ref<ast::ExprList> dims, Ref<Scope> scope) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    ArrayDimEval eval{_program, scope};
    for (unsigned n = 0; n < dims->child_num(); ++n) {
        auto expr = dims->get(n);
        auto [size, success] = eval.eval(expr);
        if (!success)
            return {};
        type = type->array_type(size);
    }
    return type;
}

std::optional<bool> Resolver::check_state(Ref<Decl> obj) {
    if (obj->state() == Decl::Resolved)
        return true;
    if (obj->state() == Decl::Resolving)
        obj->set_state(Decl::Unresolvable);
    if (obj->state() == Decl::Unresolvable)
        return false;
    obj->set_state(Decl::Resolving);
    return {};
}

void Resolver::update_state(Ref<Decl> obj, bool is_resolved) {
    obj->set_state(is_resolved ? Decl::Resolved : Decl::Unresolvable);
}

std::string_view Resolver::str(str_id_t str_id) const {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema

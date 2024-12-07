#include "libulam/diag.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/helper/param_eval.hpp>
#include <libulam/sema/resolver.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::Resolver] "
#include "src/debug.hpp"

#define CHECK_STATE(obj)                                                       \
    {                                                                          \
        auto is_resolved = check_state(obj);                                   \
        if (is_resolved.has_value())                                           \
            return is_resolved.value();                                        \
    }

namespace ulam::sema {
namespace {
ScopeProxy select_scope(Ref<ScopeObject> obj, ScopeProxy scope) {
    if (scope)
        return scope;
    PersScopeProxy proxy{obj->pers_scope_state()};
    assert(proxy);
    return proxy;
}
} // namespace

void Resolver::resolve() {
    for (auto& module : _program->modules())
        resolve(ref(module));
}

void Resolver::resolve(Ref<Module> module) {
    for (auto& pair : *module) {
        auto& [name_id, sym] = pair;
        sym.visit([&](auto&& ref_ptr) { resolve(ref_ptr.ref()); });
    }
}

bool Resolver::resolve(Ref<ClassTpl> cls_tpl) {
    CHECK_STATE(cls_tpl);
    bool is_resolved = true;
    auto params_node = cls_tpl->node()->params();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        auto name_id = param_node->name().str_id();
        auto sym = cls_tpl->get(name_id);
        assert(sym && sym->is<Var>());
        auto var = sym->get<Var>();
        auto res = resolve(var, {});
        is_resolved = is_resolved && res;
    }
    update_state(cls_tpl, is_resolved);
    return is_resolved;
}

bool Resolver::resolve(Ref<Class> cls) {
    CHECK_STATE(cls);
    bool is_resolved = true;
    PersScopeProxy scope_proxy = cls->scope()->proxy();
    scope_proxy.reset();
    for (auto name_id = scope_proxy.advance(); name_id != NoStrId;
         name_id = scope_proxy.advance()) {
        auto sym = scope_proxy.get(name_id);
        bool res{};
        if (sym->is<UserType>()) {
            // alias
            auto type = sym->get<UserType>();
            assert(type->is_alias());
            res = resolve(type->as_alias(), {});
        } else if (sym->is<Var>()) {
            // var
            res = resolve(sym->get<Var>(), {});
        } else {
            // fun
            assert(sym->is<Fun>());
            res = resolve(sym->get<Fun>());
        }
        is_resolved = is_resolved && res;
    }
    update_state(cls, is_resolved);
    return is_resolved;
}

bool Resolver::resolve(Ref<AliasType> alias, ScopeProxy scope) {
    CHECK_STATE(alias);
    auto type = resolve_type_name(
        alias->node()->type_name(), select_scope(alias, scope));
    if (type)
        alias->set_aliased(type);
    update_state(alias, (bool)type);
    return type;
}

bool Resolver::resolve(Ref<Var> var, ScopeProxy scope) {
    CHECK_STATE(var);
    // TODO: resolve references
    auto basic_type =
        resolve_type_name(var->type_node(), select_scope(var, scope));
    if (basic_type) {
        var->set_type(basic_type);
    }
    update_state(var, (bool)basic_type);
    return basic_type;
}

bool Resolver::resolve(Ref<Fun> fun) {
    CHECK_STATE(fun);
    // TODO
    return true;
}

Ref<Type> Resolver::resolve_type_name(
    ast::Ref<ast::TypeName> type_name, ScopeProxy scope) {

    // ast::TypeSpec to type
    auto type_spec = type_name->first();
    Ref<Type> type{};
    if (type_spec->type()) {
        // already has type
        type = type_spec->type();
    } else if (type_spec->type_tpl() || type_spec->cls_tpl()) {
        // non-class tpl
        // TODO: pass resolver
        ParamEval pe{_program->ast()};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = type_spec->type_tpl()->type(type_spec->args(), std::move(args));
    } else if (type_spec->cls_tpl()) {
        // class tpl
        if (!resolve(type_spec->cls_tpl()))
            return {};
        // TODO: pass resolver
        ParamEval pe{_program->ast()};
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = type_spec->cls_tpl()->type(type_spec->args(), std::move(args));
    }
    if (!type)
        return {};

    // builtin?
    if (type_spec->is_builtin()) {
        // builtins don't have members (??)
        // TODO: catch this in parser?
        if (type_name->child_num() > 1) {
            auto ident = type_name->ident(1);
            auto name_id = ident->name().str_id();
            diag().emit(
                diag::Error, ident->loc_id(), str(name_id).size(),
                "built-ins don't have member types");
        }
        return type;
    }

    // resolve rest, resolve aliases along the way
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
        if (++n == type_name->child_num())
            return type;

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

std::optional<bool> Resolver::check_state(Ref<ScopeObject> obj) {
    if (obj->res_state() == ScopeObject::Resolved)
        return true;
    if (obj->res_state() == ScopeObject::Resolving)
        obj->set_res_state(ScopeObject::Unresolvable);
    if (obj->res_state() == ScopeObject::Unresolvable)
        return false;
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

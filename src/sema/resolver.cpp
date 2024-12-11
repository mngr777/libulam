#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
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

    auto params_node = cls_tpl->node()->params();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        auto name_id = param_node->name().str_id();
        auto sym = cls_tpl->get(name_id); // TODO: don't use just symbol table
        assert(sym && sym->is<Var>());
        auto var = sym->get<Var>();
        auto res = resolve(var, {});
        is_resolved = is_resolved && res;
    }

    RET_UPD_STATE(cls_tpl, is_resolved);
}

bool Resolver::init(Ref<Class> cls) {
    bool success = true;

    // params
    {
        auto scope_proxy = cls->param_scope()->proxy();
        scope_proxy.reset();
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (!sym)
                break;
            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is_const() && var->is(Var::ClassParam));
            success = resolve(var, scope_proxy) && success;
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
                    cls->members(), false /* do not overwrite*/);
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
                diag::Error, anc.node()->loc_id(), 1, "failed to resolve");
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
    }

    RET_UPD_STATE(cls, is_resolved);
}

bool Resolver::resolve(Ref<AliasType> alias, ScopeProxy scope) {
    CHECK_STATE(alias);

    auto type_name = alias->node()->type_name();
    auto type_expr = alias->node()->type_expr();

    auto type = resolve_type_name(type_name, select_scope(alias, scope));
    if (!type)
        RET_UPD_STATE(alias, false);

    // &
    if (type_expr->is_ref() && !type->canon()->is_ref())
        type = type->ref_type();

    // []
    auto array_dims = type_expr->array_dims();
    if (array_dims) {
        assert(array_dims->child_num() > 0);
        // TODO: refactoring
        ExprVisitor ev{ast(), scope};
        for (unsigned n = 0; n < array_dims->child_num(); ++n) {
            auto dim = array_dims->get(n);
            ExprRes res = dim->accept(ev);
            auto rval = res.value().rvalue();
            // TMP
            array_size_t size{0};
            if (rval->is_unknown()) {
                diag().emit(diag::Error, dim->loc_id(), 1, "cannot calculate");
                RET_UPD_STATE(alias, false);
            } else if (rval->is<Unsigned>()) {
                size = rval->get<Unsigned>();
            } else if (rval->is<Integer>()) {
                size = rval->get<Integer>();
            } else {
                diag().emit(diag::Error, dim->loc_id(), 1, "non-numeric");
            }
            type = type->array_type(size);
        }
    }
    alias->set_aliased(type);
    RET_UPD_STATE(alias, true);
}

bool Resolver::resolve(Ref<Var> var, ScopeProxy scope) {
    CHECK_STATE(var);
    auto type = var->type();
    if (type)
        RET_UPD_STATE(var, true);

    auto base_type = resolve_type_name(var->type_node(), select_scope(var, scope));
    if (!base_type)
        RET_UPD_STATE(var, false);

    // TODO: arrays, refs
    type = base_type;

    RET_UPD_STATE(var, true);
}

bool Resolver::resolve(Ref<Fun> fun) {
    CHECK_STATE(fun);
    // 
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

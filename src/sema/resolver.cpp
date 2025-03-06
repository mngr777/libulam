#include "libulam/ast/nodes/type.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/int.hpp>

#define DEBUG_SEMA_RESOLVER // TEST
#ifdef DEBUG_SEMA_RESOLVER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Resolver] "
#    include "src/debug.hpp"
#endif

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
        module->resolve(*this);
    for (auto cls : _classes)
        resolve(cls);
    _classes.clear();
}

bool Resolver::resolve(Ref<ClassTpl> cls_tpl) {
    debug() << "resolving template " << str(cls_tpl->name_id()) << "\n";
    return cls_tpl->resolve(*this);
}

bool Resolver::init(Ref<Class> cls) {
    debug() << "initializiing " << cls->name() << "\n";
    _classes.insert(cls);
    return cls->init(*this);
}

bool Resolver::resolve(Ref<Class> cls) {
    debug() << "resolving " << cls->name() << "\n";
    return cls->resolve(*this);
}

bool Resolver::resolve(Scope::Symbol* sym, Ref<Scope> scope) {
    return sym->accept(
        [&](Ref<UserType> type) {
            if (type->is_alias())
                return resolve(type->as_alias(), scope);
            assert(type->is_class());
            return resolve(type->as_class());
        },
        [&](Ref<ClassTpl> cls_tpl) { return resolve(cls_tpl); },
        [&](auto&& other) { return resolve(other, scope); });
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
    assert(scope);
    CHECK_STATE(var);
    bool is_resolved = true;
    auto node = var->node();
    auto type_name = var->type_node();

    // type
    if (!var->has_type()) {
        auto type = resolve_var_decl_type(type_name, node, scope, true);
        if (type)
            var->set_type(type);
        is_resolved = type && is_resolved;
    }

    // value
    if (var->is_const() && var->value().empty()) {
        if (node->has_default_value()) {
            ExprVisitor ev{_program, scope};
            ExprRes res = node->default_value()->accept(ev);
            if (!res.ok())
                RET_UPD_STATE(var, false);
            // impl. cast to var type
            res = ev.cast(
                node->default_value(), var->type(), std::move(res), false);
            if (!res.ok())
                RET_UPD_STATE(var, false);
            auto tv = res.move_typed_value();
            var->set_value(tv.move_value());
            if (var->value().empty()) {
                auto name = node->name();
                diag().error(
                    name.loc_id(), str(name.str_id()).size(),
                    "cannot calculate constant value");
                is_resolved = false;
            }
        } else if (var->requires_value()) {
            auto name = node->name();
            diag().error(
                name.loc_id(), str(name.str_id()).size(),
                "constant value required");
            is_resolved = false;
        }
    }

    RET_UPD_STATE(var, is_resolved);
}

bool Resolver::resolve(Ref<Prop> prop, Ref<Scope> scope) {
    assert(scope);
    CHECK_STATE(prop);
    bool is_resolved = true;
    if (!prop->has_type()) {
        auto type =
            resolve_var_decl_type(prop->type_node(), prop->node(), scope, true);
        if (type)
            prop->set_type(type);
        is_resolved = type && is_resolved;
    }
    RET_UPD_STATE(prop, is_resolved);
}

bool Resolver::resolve(Ref<FunSet> fset, Ref<Scope> scope) {
    CHECK_STATE(fset);

    bool is_resolved = true;
    for (auto fun : *fset) {
        auto scope_view = scope->view(fun->scope_version());
        is_resolved = resolve(fun, ref(scope_view)) && is_resolved;

        // TODO: move to Class to handle inherited conversion functions

        // conversion to Int?
        if (fun->is_ready() && str(fun->name_id()) == "toInt") {
            bool is_conv = true;
            // check ret type
            auto int_type = _program->builtins().int_type();
            if (is_conv && !fun->ret_type()->is_same(int_type)) {
                diag().warn(
                    fun->node(),
                    std::string{"return type must be "} + int_type->name());
                is_conv = false;
            }
            // check params
            if (is_conv && fun->param_num() != 0) {
                diag().warn(
                    fun->node(), "conversion function cannot take params");
                is_conv = false;
            }
            if (is_conv)
                fun->cls()->add_conv(fun);
        }
    }
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
    if (ret_type) {
        fun->set_ret_type(ret_type);
    } else {
        diag().error(ret_type_node, "cannot resolve return type");
        is_resolved = false;
    }

    // params
    for (auto& param : fun->params()) {
        auto type =
            resolve_var_decl_type(param->type_node(), param->node(), scope);
        if (!type) {
            is_resolved = false;
            continue;
        }
        param->set_type(type);
    }

    RET_UPD_STATE(fun, is_resolved);
}

Ref<Type> Resolver::resolve_var_decl_type(
    Ref<ast::TypeName> type_name,
    Ref<ast::VarDecl> node,
    Ref<Scope> scope,
    bool resolve_class) {
    assert(scope);

    // base type
    auto type = resolve_type_name(type_name, scope, resolve_class);
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

Ref<Class> Resolver::resolve_class_name(
    Ref<ast::TypeName> type_name, Ref<Scope> scope, bool resolve_class) {
    auto type = resolve_type_name(type_name, scope, resolve_class);
    if (!type)
        return {};

    if (!type->is_class()) {
        diag().error(type_name, "not a class");
        return {};
    }
    if (!init(type->as_class()))
        return {};
    return type->as_class();
}

// TODO: refactor diag calls
Ref<Type> Resolver::resolve_type_name(
    Ref<ast::TypeName> type_name, Ref<Scope> scope, bool resolve_class) {
    assert(scope);

    auto type_spec = type_name->first();
    auto type = resolve_type_spec(type_spec, scope);
    if (!type)
        return {};

    // builtin?
    if (type_spec->is_builtin()) {
        if (type_name->child_num() > 1) {
            auto ident = type_name->ident(1);
            auto name_id = ident->name().str_id();
            diag().error(
                ident->loc_id(), str(name_id).size(),
                "built-ins don't have member types");
        }
        return type;
    }

    // resolve first alias in current scope
    if (type->is_alias())
        resolve(type->as_alias(), scope);

    // follow rest of type idents, resolve aliases along the way
    // e.g. in `A(x).B.C`, `A(x)` and `A(x).B` must resolve to classes,
    // `A(x).B` and `A(x).B.C` must be typedefs
    auto ident = type_name->ident(0);
    for (unsigned n = 1; n < type_name->child_num(); ++n) {
        // init next class
        if (!type->is_class()) {
            diag().error(ident, "not a class");
            return {};
        }
        auto cls = type->as_class();
        if (!init(cls)) {
            diag().error(ident, "cannot resolve");
            return {};
        }

        // resolve typedef
        ident = type_name->ident(n);
        type = cls->init_type_def(*this, ident->name_id());
        // not found ?
        if (!type) {
            diag().error(ident, "type name not found in class");
            return {};
        }
        // resolved?
        if (!type->as_alias()->is_ready())
            return {};
    }

    if (resolve_class) {
        // fully resolve class or class dependencies, e.g. array type ClassName[2]
        // depends on class ClassName
        if (!resolve_cls_deps(type)) {
            diag().error(ident, "cannot resolve");
            return {};
        }
    } else if (type->is_class()) {
        // just init if class
        if (!init(type->as_class())) {
            diag().error(ident, "cannot resolve");
            return {};
        }
    }

    return type;
}

Ref<Type>
Resolver::resolve_type_spec(Ref<ast::TypeSpec> type_spec, Ref<Scope> scope) {
    // builtin type?
    if (type_spec->is_builtin()) {
        auto bi_type_id = type_spec->builtin_type_id();
        if (!type_spec->has_args())
            return _program->builtins().type(bi_type_id);
        auto args = type_spec->args();
        assert(args->child_num() > 0);
        ExprVisitor ev{_program, scope};
        bitsize_t size = ev.bitsize_for(args->get(0), bi_type_id);
        if (size == NoBitsize)
            return {};
        if (args->child_num() > 1) {
            diag().error(args->child(1), "too many arguments");
            return {};
        }
        return _program->builtins().prim_type(bi_type_id, size);
    }
    assert(type_spec->has_ident());

    // Self?
    if (type_spec->ident()->is_self()) {
        assert(!type_spec->has_args());
        auto self_cls = scope->self_cls();
        if (!self_cls) {
            diag().error(
                type_spec->ident(), "`Self' can only be used in class context");
        }
        return self_cls;
    }

    auto name_id = type_spec->ident()->name_id();
    auto sym = scope->get(name_id);
    if (!sym) {
        diag().error(
            type_spec->ident(), std::string{str(name_id)} + " type not found");
        return {};
    }

    // template?
    if (sym->is<ClassTpl>()) {
        auto tpl = sym->get<ClassTpl>();
        if (!resolve(tpl))
            return {};
        assert(type_spec->has_args());
        ExprVisitor ev{_program, scope};
        auto [args, success] = ev.eval_tpl_args(type_spec->args(), tpl);
        if (!success)
            return {};
        return tpl->type(std::move(args));
    }
    assert(sym->is<UserType>());
    return sym->get<UserType>();
}

bool Resolver::resolve_cls_deps(Ref<Type> type) {
    type = type->canon();
    if (type->is_class())
        return resolve(type->as_class());
    if (type->is_array())
        return resolve_cls_deps(type->as_array()->item_type());
    // NOTE: not resolving referenced classes
    return true;
}

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type, Ref<ast::ExprList> dims, Ref<Scope> scope) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    ExprVisitor ev{_program, scope};
    for (unsigned n = 0; n < dims->child_num(); ++n) {
        array_idx_t size = ev.array_index(dims->get(n));
        if (size == UnknownArraySize)
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

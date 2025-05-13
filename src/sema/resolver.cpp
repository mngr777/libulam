#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/void.hpp>

#ifdef DEBUG_SEMA_RESOLVER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Resolver] "
#endif
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

void Resolver::resolve(Ref<Program> program) {
    for (auto& module : program->modules())
        module->resolve(*this);

    // TODO: loop
    for (auto cls : _classes)
        resolve(cls);
    for (auto cls : _classes) {
        for (auto type_def : cls->type_defs())
            resolve(type_def);
        for (auto var : cls->consts())
            resolve(var);
    }
    _classes.clear();
}

bool Resolver::init(Ref<Class> cls) {
    debug() << "initializiing " << cls->name() << "\n";
    _classes.insert(cls);
    return cls->init(*this);
}

bool Resolver::resolve(Ref<Class> cls) {
    debug() << "resolving " << cls->name() << "\n";
    bool ok = cls->resolve(*this);

    // check if size limit is exceeded
    if (ok && cls->required_bitsize() > cls->bitsize()) {
        cls->set_state(Decl::Unresolvable);
        auto prop = cls->first_prop_over_max_bitsize();
        if (prop) {
            auto message = std::string{"size limit of "} +
                           std::to_string(cls->bitsize()) + " bits exceeded";
            _diag.error(prop->node(), std::move(message));
            return false;
        }
        auto parent = cls->first_parent_over_max_bitsize();
        if (parent) {
            _diag.error(parent->node(), "size limit exceeded");
            return false;
        }
        assert(false);
    }

    if (ok)
        _classes.insert(cls);
    return ok;
}

bool Resolver::resolve(Ref<AliasType> alias, Scope* scope) {
    CHECK_STATE(alias);
    auto type_name = alias->node()->type_name();
    auto type_expr = alias->node()->type_expr();

    PersScopeView scope_view{};
    if (!alias->is_local()) {
        scope_view = decl_scope_view(alias);
        scope = &scope_view;
    }
    assert(scope);

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

bool Resolver::resolve(Ref<Var> var, Scope* scope) {
    CHECK_STATE(var);
    auto node = var->node();
    auto type_name = var->type_node();

    PersScopeView scope_view{};
    if (!var->is_local()) {
        scope_view = decl_scope_view(var);
        scope = &scope_view;
    }
    assert(scope);

    // type
    if (!var->has_type()) {
        auto type = resolve_var_decl_type(type_name, node, scope, true);
        if (!var->is_local() && type->is_ref()) {
            if (var->has_cls()) {
                _diag.error(
                    type_name, "class constant cannot have reference type");
            } else {
                assert(var->has_module());
                _diag.error(
                    type_name, "module constant cannot have reference type");
            }
            type = {};
        }
        if (!type)
            RET_UPD_STATE(var, false);
        var->set_type(type);
    }

    // value
    if (!var->has_value()) {
        if (node->has_init()) {
            auto flags = _flags;
            if (!var->is_local() && !var->type()->is_ref())
                flags |= evl::Consteval; // class/module const
            auto init = _eval.init_helper(scope, flags);
            auto init_res = init->eval_init(var, node->init());
            bool ok = init_res.ok();
            update_state(var, ok);
            if (ok)
                _eval.var_init_expr(var, std::move(init_res), _in_expr);
            return ok;

        } else if (var->requires_value()) {
            auto name = node->name();
            _diag.error(
                name.loc_id(), str(name.str_id()).size(), "value required");
            RET_UPD_STATE(var, false);

        } else {
            update_state(var, true);
            _eval.var_init_default(var, _in_expr);
            return true;
        }
    }
    RET_UPD_STATE(var, true);
}

bool Resolver::resolve(Ref<AliasType> alias) { return resolve(alias, {}); }

bool Resolver::resolve(Ref<Var> var) { return resolve(var, {}); }

bool Resolver::resolve(Ref<Prop> prop) {
    CHECK_STATE(prop);

    auto scope_view = decl_scope_view(prop);

    // type
    auto type = resolve_var_decl_type(
        prop->type_node(), prop->node(), &scope_view, true);
    if (!type)
        RET_UPD_STATE(prop, false);
    // TODO: more type checks, e.g. for Void
    if (type->canon()->is_ref()) {
        _diag.error(prop->type_node(), "property cannot have a reference type");
        RET_UPD_STATE(prop, false);
    }
    prop->set_type(type);

    RET_UPD_STATE(prop, true);
}

bool Resolver::init_default_value(Ref<Prop> prop) {
    if (!resolve(prop))
        return false;

    if (prop->has_default_value())
        return true;

    auto type = prop->type();
    auto node = prop->node();
    auto scope_view = decl_scope_view(prop);

    if (node->has_init()) {
        auto flags = _flags | evl::Consteval;
        auto init = _eval.init_helper(&scope_view, flags);
        auto init_res = init->eval_init(prop, node->init());
        if (!init_res)
            RET_UPD_STATE(prop, false);
        prop->set_default_value(init_res.move_value().move_rvalue());
    } else {
        prop->set_default_value(type->construct());
    }
    return true;
}

bool Resolver::resolve(Ref<FunSet> fset) {
    CHECK_STATE(fset);
    if (fset->empty())
        RET_UPD_STATE(fset, true);

    bool is_resolved = true;
    for (auto fun : *fset) {
        is_resolved = resolve(fun) && is_resolved;

        // conversion to Int?
        if (fun->is_ready() && str(fun->name_id()) == "toInt") {
            bool is_conv = true;
            // check ret type
            auto int_type = _builtins.int_type();
            if (is_conv && !fun->ret_type()->is_same(int_type)) {
                _diag.warn(
                    fun->node(),
                    std::string{"return type must be "} + int_type->name());
                is_conv = false;
            }
            // check params
            if (is_conv && fun->param_num() != 0) {
                _diag.warn(
                    fun->node(), "conversion function cannot take params");
                is_conv = false;
            }
            if (is_conv)
                fun->cls()->add_conv(fun);
        }
    }
    fset->init_map(_diag, _str_pool);
    RET_UPD_STATE(fset, is_resolved);
}

bool Resolver::resolve(Ref<Fun> fun) {
    CHECK_STATE(fun);
    bool is_resolved = true;

    auto scope_view = decl_scope_view(fun);

    // return type
    if (fun->is_constructor()) {
        fun->set_ret_type(_builtins.void_type());
    } else {
        auto ret_type_node = fun->ret_type_node();
        auto ret_type = resolve_fun_ret_type(ret_type_node, &scope_view);
        if (ret_type) {
            fun->set_ret_type(ret_type);
        } else {
            _diag.error(ret_type_node, "cannot resolve return type");
            is_resolved = false;
        }
    }

    // params
    for (auto& param : fun->params()) {
        auto type = resolve_var_decl_type(
            param->type_node(), param->node(), &scope_view);
        if (!type) {
            is_resolved = false;
            continue;
        }
        param->set_type(type);
    }

    RET_UPD_STATE(fun, is_resolved);
}

Ref<Class> Resolver::resolve_class_name(
    Ref<ast::TypeName> type_name, Scope* scope, bool resolve_class) {
    auto type = resolve_type_name(type_name, scope, resolve_class);
    if (!type)
        return {};

    if (!type->is_class()) {
        _diag.error(type_name, "not a class");
        return {};
    }
    if (!init(type->as_class()))
        return {};
    return type->as_class();
}

Ref<Type> Resolver::resolve_full_type_name(
    Ref<ast::FullTypeName> full_type_name,
    Scope* scope,
    bool resolve_class) {

    // type
    auto type =
        resolve_type_name(full_type_name->type_name(), scope, resolve_class);
    if (!type)
        return {};

    // []
    if (full_type_name->has_array_dims())
        type = apply_array_dims(type, full_type_name->array_dims(), scope);
    if (!type)
        return {};

    // &
    if (full_type_name->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type> Resolver::resolve_type_name(
    Ref<ast::TypeName> type_name, Scope* scope, bool resolve_class) {
    auto type_spec = type_name->first();
    auto type = resolve_type_spec(type_spec, scope);
    if (!type)
        return {};

    // builtin?
    if (type_spec->is_builtin()) {
        if (type_name->child_num() > 1) {
            auto ident = type_name->ident(1);
            auto name_id = ident->name().str_id();
            _diag.error(
                ident->loc_id(), str(name_id).size(),
                "built-ins don't have member types");
        }
        return type;
    }

    // resolve first alias in current scope
    if (type->is_alias() && !resolve(type->as_alias(), scope))
        return {};

    // follow rest of type idents, resolve aliases along the way
    // e.g. in `A(x).B.C`, `A(x)` and `A(x).B` must resolve to classes,
    // `A(x).B` and `A(x).B.C` must be typedefs (or base classes)
    auto ident = type_name->ident(0);
    for (unsigned n = 1; n < type_name->child_num(); ++n) {
        // init next class
        if (!type->is_class()) {
            _diag.error(ident, "not a class");
            return {};
        }
        auto cls = type->as_class();
        if (!init(cls)) {
            _diag.error(ident, "cannot resolve");
            return {};
        }

        // resolve typedef
        ident = type_name->ident(n);
        if (ident->is_super()) {
            if (!cls->has_super()) {
                _diag.error(ident, "class doesn't have a superclass");
                return {};
            }
            type = cls->super();
        } else {
            assert(!ident->is_self());
            // alias?
            type = cls->init_type_def(*this, ident->name_id());
            if (!type) {
                // base?
                type = cls->base_by_name_id(ident->name_id());
            }
        }
        // not found?
        if (!type) {
            _diag.error(ident, "type name not found in class");
            return {};
        }
        // resolved?
        if (type->is_alias() && !type->as_alias()->is_ready())
            return {};
    }

    if (resolve_class) {
        // fully resolve class or class dependencies, e.g. array type
        // ClassName[2] depends on class ClassName
        if (!resolve_class_deps(type)) {
            _diag.error(ident, "cannot resolve");
            return {};
        }
    } else if (type->is_class()) {
        // just init if class
        if (!init(type->as_class())) {
            _diag.error(ident, "cannot resolve");
            return {};
        }
    }

    return type;
}

Ref<Type>
Resolver::resolve_type_spec(Ref<ast::TypeSpec> type_spec, Scope* scope) {
    // builtin type?
    if (type_spec->is_builtin()) {
        auto bi_type_id = type_spec->builtin_type_id();
        if (!type_spec->has_args())
            return _builtins.type(bi_type_id);
        auto args = type_spec->args();
        assert(args->child_num() > 0);
        auto flags = _flags | evl::Consteval;
        auto ev = _eval.expr_visitor(scope, flags);
        bitsize_t size = ev->bitsize_for(args->get(0), bi_type_id);
        if (size == NoBitsize)
            return {};
        if (args->child_num() > 1) {
            _diag.error(args->child(1), "too many arguments");
            return {};
        }
        return _builtins.prim_type(bi_type_id, size);
    }
    assert(type_spec->has_ident());
    auto ident = type_spec->ident();

    // Self or Super?
    if (ident->is_self() || ident->is_super()) {
        assert(!type_spec->has_args());
        auto self_cls = scope->ctx().self_cls();
        if (!self_cls) {
            std::string name{ident->is_self() ? "Self" : "Super"};
            _diag.error(ident, name + " can only be used in class context");
            return {};
        }
        // Self
        if (ident->is_self())
            return self_cls;

        // Super
        if (!self_cls->has_super()) {
            _diag.error(
                ident, self_cls->name() + " does not have a superclass");
            return {};
        }
        return self_cls->super();
    }

    auto name_id = ident->name_id();
    auto sym =
        ident->is_local() ? scope->get_local(name_id) : scope->get(name_id);
    if (!sym) {
        _diag.error(ident, std::string{str(name_id)} + " type not found");
        return {};
    }

    // template?
    if (sym->is<ClassTpl>()) {
        auto tpl = sym->get<ClassTpl>();
        auto flags = _flags | evl::Consteval;
        auto ev = _eval.expr_visitor(scope, flags);
        auto [args, success] = ev->eval_tpl_args(type_spec->args(), tpl);
        if (!success)
            return {};
        return tpl->type(std::move(args));
    }
    assert(sym->is<UserType>());
    return sym->get<UserType>();
}

PersScopeView Resolver::decl_scope_view(Ref<Decl> decl) {
    assert(!decl->is_local());
    auto scope =
        decl->has_cls() ? decl->cls()->scope() : decl->module()->scope();
    return scope->view(decl->scope_version());
}

bool Resolver::resolve_class_deps(Ref<Type> type) {
    type = type->canon();
    if (type->is_class())
        return resolve(type->as_class());
    if (type->is_array())
        return resolve_class_deps(type->as_array()->item_type());
    // NOTE: not resolving referenced classes
    return true;
}

Ref<Type> Resolver::resolve_var_decl_type(
    Ref<ast::TypeName> type_name,
    Ref<ast::VarDecl> node,
    Scope* scope,
    bool resolve_class) {
    assert(scope);

    // base type
    auto type = resolve_type_name(type_name, scope, resolve_class);
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(type, node->array_dims(), node->init(), scope);

    // &
    if (node->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type>
Resolver::resolve_fun_ret_type(Ref<ast::FunRetType> node, Scope* scope) {
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

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type,
    Ref<ast::ExprList> dims,
    Ref<ast::InitValue> init,
    Scope* scope) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    assert(!dims->has_empty() || init);

    // get dimension list
    ArrayDimList dim_list;
    if (dims->has_empty()) {
        if (!init) {
            _diag.error(
                dims, "array size must be set explicitly unless initializer "
                      "list is provided");
            return {};
        }
        bool ok = false;
        auto flags = _flags | evl::Consteval;
        auto init_helper = _eval.init_helper(scope, flags);
        std::tie(dim_list, ok) =
            init_helper->array_dims(dims->child_num(), init);
        if (!ok)
            return {};
    }

    // make array type
    auto flags = _flags | evl::Consteval;
    auto ev = _eval.expr_visitor(scope, flags);
    for (unsigned n = 0; n < dims->child_num(); ++n) {
        auto expr = dims->get(n);
        array_size_t size{};
        if (expr) {
            // explicit size expr
            size = ev->array_size(expr);
            if (size == UnknownArraySize)
                return {};
        } else {
            // size defined by init list
            assert(n < dim_list.size());
            size = dim_list[n];
        }
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
    return _str_pool.get(str_id);
}

} // namespace ulam::sema

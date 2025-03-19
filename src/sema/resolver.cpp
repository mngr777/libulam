#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/void.hpp>

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
    bool ok = cls->resolve(*this);

    // check if size limit is exceeded
    if (ok && cls->required_bitsize() > cls->bitsize()) {
        cls->set_state(Decl::Unresolvable);
        auto prop = cls->first_prop_over_max_bitsize();
        if (prop) {
            auto message = std::string{"size limit of "} +
                           std::to_string(cls->bitsize()) + " bits exceeded";
            diag().error(prop->node(), std::move(message));
            return false;
        }
        auto parent = cls->first_parent_over_max_bitsize();
        if (parent) {
            diag().error(parent->node(), "size limit exceeded");
            return false;
        }
        assert(false);
    }
    return ok;
}

bool Resolver::resolve(Ref<AliasType> alias, Ref<Scope> scope) {
    CHECK_STATE(alias);
    auto type_name = alias->node()->type_name();
    auto type_expr = alias->node()->type_expr();

    Ptr<PersScopeView> scope_view{};
    if (alias->has_module()) {
        scope_view = decl_scope_view(alias);
        scope = ref(scope_view);
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

bool Resolver::resolve(Ref<Var> var, Ref<Scope> scope) {
    CHECK_STATE(var);
    bool is_resolved = true;
    auto node = var->node();
    auto type_name = var->type_node();

    Ptr<PersScopeView> scope_view{};
    if (var->has_module()) {
        scope_view = decl_scope_view(var);
        scope = ref(scope_view);
    }
    assert(scope);

    // type
    if (!var->has_type()) {
        auto type = resolve_var_decl_type(type_name, node, scope, true);
        if (type)
            var->set_type(type);
        is_resolved = type && is_resolved;
    }

    // value
    if (var->requires_value()) {
        if (node->has_init()) {
            ExprVisitor ev{_program, scope};
            auto [val, ok] = ev.eval_init(var->node(), var->type());
            if (ok)
                var->set_value(std::move(val));
            is_resolved = is_resolved && ok;

        } else {
            auto name = node->name();
            diag().error(
                name.loc_id(), str(name.str_id()).size(),
                "constant value required");
            is_resolved = false;
        }
    }
    RET_UPD_STATE(var, is_resolved);
}

bool Resolver::resolve(Ref<AliasType> alias) { return resolve(alias, {}); }

bool Resolver::resolve(Ref<Var> var) { return resolve(var, {}); }

bool Resolver::resolve(Ref<Prop> prop) {
    CHECK_STATE(prop);
    bool is_resolved = true;
    if (!prop->has_type()) {
        auto scope_view = decl_scope_view(prop);
        auto type = resolve_var_decl_type(
            prop->type_node(), prop->node(), ref(scope_view), true);
        if (type)
            prop->set_type(type);
        is_resolved = type && is_resolved;
    }
    RET_UPD_STATE(prop, is_resolved);
}

bool Resolver::resolve(Ref<FunSet> fset) {
    CHECK_STATE(fset);

    bool is_resolved = true;
    for (auto fun : *fset) {
        is_resolved = resolve(fun) && is_resolved;

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

bool Resolver::resolve(Ref<Fun> fun) {
    CHECK_STATE(fun);
    bool is_resolved = true;

    auto scope_view = decl_scope_view(fun);

    // return type
    if (fun->is_constructor()) {
        fun->set_ret_type(_program->builtins().void_type());
    } else {
        auto ret_type_node = fun->ret_type_node();
        auto ret_type = resolve_fun_ret_type(ret_type_node, ref(scope_view));
        if (ret_type) {
            fun->set_ret_type(ret_type);
        } else {
            diag().error(ret_type_node, "cannot resolve return type");
            is_resolved = false;
        }
    }

    // params
    for (auto& param : fun->params()) {
        auto type = resolve_var_decl_type(
            param->type_node(), param->node(), ref(scope_view));
        if (!type) {
            is_resolved = false;
            continue;
        }
        param->set_type(type);
    }

    RET_UPD_STATE(fun, is_resolved);
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

Ref<Type> Resolver::resolve_full_type_name(
    Ref<ast::FullTypeName> full_type_name,
    Ref<Scope> scope,
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
    Ref<ast::TypeName> type_name, Ref<Scope> scope, bool resolve_class) {
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
    if (type->is_alias()) {
        if (!resolve(type->as_alias(), scope))
            return {};
    }

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
        if (ident->is_super()) {
            if (!cls->has_super()) {
                diag().error(ident, "class doesn't have a superclass");
                return {};
            }
            type = cls->super();
        } else {
            assert(!ident->is_self());
            type = cls->init_type_def(*this, ident->name_id());
        }
        // not found ?
        if (!type) {
            diag().error(ident, "type name not found in class");
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
    auto ident = type_spec->ident();

    // Self or Super?
    if (ident->is_self() || ident->is_super()) {
        assert(!type_spec->has_args());
        auto self_cls = scope->self_cls();
        if (!self_cls) {
            std::string name{ident->is_self() ? "Self" : "Super"};
            diag().error(ident, name + " can only be used in class context");
            return {};
        }
        // Self
        if (ident->is_self())
            return self_cls;

        // Super
        if (!self_cls->has_super()) {
            diag().error(
                ident, self_cls->name() + " does not have a superclass");
            return {};
        }
        return self_cls->super();
    }

    auto name_id = ident->name_id();
    auto sym =
        ident->is_local() ? scope->get_local(name_id) : scope->get(name_id);
    if (!sym) {
        diag().error(ident, std::string{str(name_id)} + " type not found");
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

Ptr<PersScopeView> Resolver::decl_scope_view(Ref<Decl> decl) {
    assert(decl->has_cls() || decl->has_module());
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
    Ref<Scope> scope,
    bool resolve_class) {
    assert(scope);

    // base type
    auto type = resolve_type_name(type_name, scope, resolve_class);
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(
            type, node->array_dims(), node->init_list(), scope);

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

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type,
    Ref<ast::ExprList> dims,
    Ref<ast::InitList> init_list,
    Ref<Scope> scope) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    assert(!dims->has_empty() || init_list);

    auto num = dims->child_num();
    if (init_list) {
        // does the number of dimensions match init list's?
        auto depth = init_list->depth();
        if (num != depth) {
            assert(num > 0);
            Ref<ast::Node> node =
                (num < depth) ? dims->child(num - 1) : dims->child(depth);
            auto message = std::string{"array type has "} +
                           std::to_string(num) +
                           " dimensions while initializer list has " +
                           std::to_string(depth);
            diag().error(node, std::move(message));
            return {};
        }
    }

    ExprVisitor ev{_program, scope};
    for (unsigned n = 0; n < num; ++n) {
        auto expr = dims->get(n);
        if (expr) {
            // eval array size expr
            array_size_t size = ev.array_size(dims->get(n));
            if (size == UnknownArraySize)
                return {};

            // matches init list?
            // (one-dimensional lists are allowed to have less elements)
            if (init_list) {
                if ((num > 1 && size > init_list->child_num()) ||
                    size < init_list->child_num()) {
                    auto message = std::string{"array size "} +
                                   std::to_string(size) +
                                   " does not match initializer list (" +
                                   std::to_string(init_list->child_num()) + ")";
                    diag().error(expr, std::move(message));
                    return {};
                }
            }
            type = type->array_type(size);

        } else {
            // use size from init list
            if (!init_list) {
                diag().error(
                    dims, "array size must be set explicitly unless "
                          "initializer list is provided");
                return {};
            }
            type = type->array_type(init_list->child_num());
        }

        if (init_list && n + 1 < num) {
            // move to first sublist
            init_list = init_list->sublist(0);
            assert(init_list);
        }
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

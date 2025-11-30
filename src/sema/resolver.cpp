#include "libulam/semantic/scope.hpp"
#include "libulam/semantic/type.hpp"
#include <libulam/sema/class_resolver.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/class_tpl.hpp>

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

#define DECL_SCOPE(decl, ssr, scope_view)                                      \
    EvalEnv::ScopeSwitchRaii ssr;                                              \
    PersScopeView scope_view;                                                  \
    if (!decl->is_local()) {                                                   \
        scope_view = decl_scope_view(decl);                                    \
        ssr = env().scope_switch_raii(&scope_view);                            \
    }                                                                          \
    do {                                                                       \
    } while (false)

namespace ulam::sema {

void Resolver::resolve(Ref<Program> program) {
    for (auto& module : program->modules())
        module->resolve(*this);

    ClassSet processed;
    while (true) {
        ClassSet classes;
        std::swap(classes, _classes);

        ClassSet processing;
        for (auto cls : classes) {
            if (processed.count(cls) > 0)
                continue;
            if (ClassResolver{env(), *this, *cls}.resolve())
                processing.insert(cls);
        }
        if (processing.empty())
            break;

        for (auto cls : processing) {
            ClassResolver{env(), *this, *cls}.resolve(true);
            processed.insert(cls);
        }
    }

    debug() << "fully resolved classes:\n";
    for (auto cls : processed)
        debug() << " - " << cls->full_name() << "\n";
}

bool Resolver::init(Ref<Class> cls) {
    debug() << "initializing " << cls->name() << "\n";
    bool ok = ClassResolver{env(), *this, *cls}.init();
    if (ok)
        _classes.insert(cls);
    return ok;
}

bool Resolver::resolve(Ref<Class> cls) {
    debug() << "resolving " << cls->name() << "\n";
    bool ok = ClassResolver{env(), *this, *cls}.resolve();
    if (ok)
        _classes.insert(cls);
    return ok;
}

bool Resolver::resolve(Ref<AliasType> alias) {
    CHECK_STATE(alias);
    auto type_name = alias->node()->type_name();
    auto type_expr = alias->node()->type_expr();

    // type
    DECL_SCOPE(alias, ssr, scope_view);
    Ref<Type> type = do_resolve_type_name(type_name, alias, false);
    if (!type)
        RET_UPD_STATE(alias, false);

    // []
    if (type_expr->has_array_dims())
        type = apply_array_dims(type, type_expr->array_dims());
    if (!type)
        RET_UPD_STATE(alias, false);

    // &
    if (type_expr->is_ref())
        type = type->ref_type();

    alias->set_aliased(type);
    RET_UPD_STATE(alias, true);
}

bool Resolver::resolve(Ref<Var> var) {
    CHECK_STATE(var);
    auto node = var->node();
    auto type_name = var->type_node();

    DECL_SCOPE(var, ssr, scope_view);

    // type
    if (!var->has_type()) {
        auto type = resolve_var_decl_type(type_name, node, true);
        if (!var->is_local() && type->is_ref()) {
            if (var->has_cls()) {
                diag().error(
                    type_name, "class constant cannot have reference type");
            } else {
                assert(var->has_module());
                diag().error(
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
        auto arg = env().move_var_default(var);
        if (arg) {
            bool ok = env().init_var_with(var, std::move(arg));
            RET_UPD_STATE(var, ok);
        }

        if (node->has_init()) {
            EvalEnv::FlagsRaii fr{};
            if (!var->is_local() && !var->type()->is_ref())
                fr = env().add_flags_raii(evl::Consteval); // class/module const
            bool ok = env().init_var(var, node->init(), _in_expr);
            RET_UPD_STATE(var, ok);
        }

        if (var->requires_value()) {
            auto name = node->name();
            diag().error(
                name.loc_id(), str(name.str_id()).size(), "value required");
            RET_UPD_STATE(var, false);
        }

        env().init_var(var, {}, _in_expr);
    }
    RET_UPD_STATE(var, true);
}

bool Resolver::resolve(Ref<Prop> prop) {
    CHECK_STATE(prop);

    DECL_SCOPE(prop, ssr, scope_view);

    // type
    auto type = resolve_var_decl_type(prop->type_node(), prop->node(), true);
    if (!type)
        RET_UPD_STATE(prop, false);
    // TODO: more type checks, e.g. for Void
    if (type->canon()->is_ref()) {
        diag().error(
            prop->type_node(), "property cannot have a reference type");
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

    DECL_SCOPE(prop, ssr, scope_view);

    auto fr = env().add_flags_raii(evl::Consteval);
    bool ok = env().init_prop(prop, prop->node()->init());
    RET_UPD_STATE(prop, ok);
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
            auto int_type = builtins().int_type();
            if (is_conv && !fun->ret_type()->is_same(int_type)) {
                diag().warn(
                    fun->node(),
                    "return type must be " + std::string{int_type->name()});
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
    if (is_resolved)
        fset->init_map(diag(), str_pool());
    RET_UPD_STATE(fset, is_resolved);
}

bool Resolver::resolve(Ref<Fun> fun) {
    CHECK_STATE(fun);
    bool is_resolved = true;

    DECL_SCOPE(fun, ssr, scope_view);

    // return type
    if (fun->is_constructor()) {
        fun->set_ret_type(builtins().void_type());
    } else {
        auto ret_type_node = fun->ret_type_node();
        auto ret_type = resolve_fun_ret_type(ret_type_node);
        if (ret_type) {
            fun->set_ret_type(ret_type);
        } else {
            diag().error(ret_type_node, "cannot resolve return type");
            is_resolved = false;
        }
    }

    // params
    for (auto& param : fun->params()) {
        auto type = resolve_var_decl_type(param->type_node(), param->node());
        if (!type) {
            is_resolved = false;
            continue;
        }
        param->set_type(type);
    }

    RET_UPD_STATE(fun, is_resolved);
}

Ref<Class>
Resolver::resolve_class_name(Ref<ast::TypeName> type_name, bool resolve_class) {
    auto type = resolve_type_name(type_name, resolve_class);
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
    Ref<ast::FullTypeName> full_type_name, bool resolve_class) {

    // type
    auto type = resolve_type_name(full_type_name->type_name(), resolve_class);
    if (!type)
        return {};

    // []
    if (full_type_name->has_array_dims())
        type = apply_array_dims(type, full_type_name->array_dims());
    if (!type)
        return {};

    // &
    if (full_type_name->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type>
Resolver::resolve_type_name(Ref<ast::TypeName> type_name, bool resolve_class) {
    return do_resolve_type_name(type_name, {}, resolve_class);
}

Ref<Type>
Resolver::resolve_type_spec(Ref<ast::TypeSpec> type_spec, bool resolve_class) {
    return do_resolve_type_spec(type_spec, {}, resolve_class);
}

Ref<Type> Resolver::do_resolve_type_name(
    Ref<ast::TypeName> type_name,
    Ref<AliasType> exclude_alias,
    bool resolve_class) {
    auto type_spec = type_name->first();
    auto type = do_resolve_type_spec(type_spec, exclude_alias, false);
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
    if (type->is_alias() && !resolve(type->as_alias()))
        return {};

    // follow rest of type idents, resolve aliases along the way
    // e.g. in `A(x).B.C`, `A(x)` and `A(x).B` must resolve to classes,
    // `A(x).B` and `A(x).B.C` must be typedefs (or base classes)
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
        const auto name_id = ident->name_id();
        if (ident->is_super()) {
            if (!cls->has_super()) {
                diag().error(ident, "class doesn't have a superclass");
                return {};
            }
            type = cls->super();
        } else {
            assert(!ident->is_self());
            // alias or base class?
            auto sym = cls->get(name_id);
            if (sym) {
                // alias
                type = sym->get<UserType>();
                if (!resolve(type->as_alias()))
                    return {};
            } else {
                // base?
                // TODO: allow type specs somehow and remove
                // ast::BaseTypeSelect??
                type = cls->base_by_name_id(name_id);
            }
            type = sym ? sym->get<UserType>() : cls->base_by_name_id(name_id);
        }
        // not found?
        if (!type) {
            diag().error(ident, "type name not found in class");
            return {};
        }
        // resolved?
        if (type->is_alias() && !type->as_alias()->is_ready())
            return {};
    }

    if (type && prepare_resolved_type(ident, type, resolve_class))
        return type;
    return {};
}

Ref<Type> Resolver::do_resolve_type_spec(
    Ref<ast::TypeSpec> type_spec,
    Ref<AliasType> exclude_alias,
    bool resolve_class) {

    // builtin type?
    if (type_spec->is_builtin()) {
        auto bi_type_id = type_spec->builtin_type_id();
        if (!type_spec->has_args())
            return builtins().type(bi_type_id);

        auto args = type_spec->args();
        assert(args->child_num() > 0);
        bitsize_t size = bitsize_for(args->get(0), bi_type_id);
        if (size == NoBitsize)
            return {};
        if (args->child_num() > 1) {
            diag().error(args->child(1), "too many arguments");
            return {};
        }
        return builtins().prim_type(bi_type_id, size);
    }
    assert(type_spec->has_ident());
    auto ident = type_spec->ident();

    // Self or Super?
    if (ident->is_self() || ident->is_super()) {
        assert(!type_spec->has_args());
        auto self_cls = scope()->self_cls();
        if (!self_cls) {
            std::string name{ident->is_self() ? "Self" : "Super"};
            diag().error(ident, name + " can only be used in class context");
            return {};
        }
        // Self
        if (ident->is_self()) {
            if (prepare_resolved_type(ident, self_cls, resolve_class))
                return self_cls;
            return {};
        }

        // Super
        if (!self_cls->has_super()) {
            diag().error(
                ident,
                std::string{self_cls->name()} + " does not have a superclass");
            return {};
        }
        if (prepare_resolved_type(ident, self_cls->super(), resolve_class))
            return self_cls->super();
        return {};
    }

    bool is_tpl = type_spec->has_args();
    auto name_id = ident->name_id();
    auto class_tpl_to_type = [&](Ref<ClassTpl> class_tpl) -> Ref<Type> {
        if (!is_tpl) {
            diag().error(type_spec, "no template arguments provided");
            return {};
        }
        auto [args, success] = eval_tpl_args(type_spec->args(), class_tpl);
        if (!success)
            return {};
        auto type = class_tpl->type(std::move(args));
        if (type && prepare_resolved_type(ident, type, resolve_class))
            return type;
        return {};
    };

    {
        Scope::GetParams sgp;
        sgp.local = ident->is_local();
        sgp.except = exclude_alias;
        auto sym = scope()->get(name_id, sgp);
        if (sym) {
            auto type = sym->accept(
                class_tpl_to_type,
                [&](Ref<UserType> type) -> Ref<Type> {
                    // NOTE:
                    // * class ancestor can be added into class scope
                    //   under same name as template:
                    //   ```
                    //   quark A(Int cP) { Void foo() {<...>} }
                    //   quark B : A(1) {
                    //     A.foo(); // `A` resolves to `A(1)` in class scope
                    //   }
                    //   ```
                    // * same for typedef shadowing template (see t3651,
                    // broken?):
                    //   ```
                    //   quark A(Int cp) {}
                    //   typedef A(0) A;
                    //   ```
                    if (is_tpl || !type->is_alias())
                        return {};

                    assert(type->is_alias());
                    if (!resolve(type->as_alias()))
                        return {};
                    if (prepare_resolved_type(ident, type, resolve_class))
                        return type;
                    return {};
                },
                [&](auto&&) -> Ref<Type> { assert(false); });
            if (type) {
                if (prepare_resolved_type(ident, type, resolve_class))
                    return type;
                return {};
            }
        }
    }

    auto exp = program()->exports().get(name_id);
    if (!exp) {
        diag().error(ident, std::string{str(name_id)} + " type not found");
        return {};
    }

    // import into module, TODO: add and use Module::add_import instead
    auto module = scope()->module();
    auto type = exp->sym()->accept(
        [&](Ref<ClassTpl> class_tpl) -> Ref<Type> {
            if (module)
                module->env_scope()->set(name_id, class_tpl); // TODO
            return class_tpl_to_type(class_tpl);
        },
        [&](Ref<Class> cls) -> Ref<Type> {
            if (module)
                module->env_scope()->set(name_id, cls); // TODO
            return cls;
        },
        [&](auto&&) -> Ref<Type> { assert(false); });
    if (type && prepare_resolved_type(ident, type, resolve_class))
        return type;
    return {};
}

bitsize_t Resolver::bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id) {
    debug() << __FUNCTION__ << "\n" << line_at(expr);
    assert(bi_type_id != NoBuiltinTypeId);

    // can have bitsize?
    if (!has_bitsize(bi_type_id)) {
        diag().error(
            expr, std::string{builtin_type_str(bi_type_id)} +
                      " does not have bitsize parameter");
        return NoBitsize;
    }

    // consteval
    auto fr = env().add_flags_raii(evl::Consteval);

    // eval
    ExprRes res = env().eval_expr(expr);
    if (!res)
        return NoBitsize;

    // cast to Unsigned
    Unsigned size{};
    {
        auto uns_type = builtins().unsigned_type();
        res = env().cast(expr, uns_type, std::move(res), true);
        if (!res)
            return NoBitsize;
        auto rval = res.move_value().move_rvalue();
        size = rval.get<Unsigned>();
    }

    // check range
    auto tpl = builtins().prim_type_tpl(bi_type_id);
    if (size < tpl->min_bitsize()) {
        auto message = std::string{"min size for "} +
                       std::string{builtin_type_str(bi_type_id)} + " is " +
                       std::to_string(tpl->min_bitsize());
        diag().error(expr, message);
        return NoBitsize;
    }
    if (size > tpl->max_bitsize()) {
        auto message = std::string{"max size for "} +
                       std::string{builtin_type_str(bi_type_id)} + " is " +
                       std::to_string(tpl->max_bitsize());
        diag().error(expr, message);
        return NoBitsize;
    }
    return size;
}

PersScopeView Resolver::decl_scope_view(Ref<Decl> decl) {
    assert(!decl->is_local());
    Ref<PersScope> scope{};
    if (decl->has_cls()) {
        // TODO: params, fun params, ...
        scope = decl->cls()->scope();
    } else if (decl->has_module()) {
        scope = decl->module()->scope();
    } else {
        assert(false);
    }
    return scope->view(decl->scope_version());
}

bool Resolver::prepare_resolved_type(
    Ref<ast::TypeIdent> ident, Ref<Type> type, bool resolve_class) {
    if (resolve_class) {
        // fully resolve class or class dependencies, e.g. array type
        // ClassName[2] depends on class ClassName
        if (!resolve_class_deps(type)) {
            diag().error(ident, "cannot resolve");
            return false;
        }
    } else if (type->is_class()) {
        // just init if class
        if (!init(type->as_class())) {
            diag().error(ident, "cannot resolve");
            return false;
        }
    }
    return true;
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
    Ref<ast::TypeName> type_name, Ref<ast::VarDecl> node, bool resolve_class) {

    // base type
    auto type = resolve_type_name(type_name, resolve_class);
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(type, node->array_dims(), node->init());

    // &
    if (node->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type> Resolver::resolve_fun_ret_type(Ref<ast::FunRetType> node) {
    // base type
    auto type = resolve_type_name(node->type_name());
    if (!type)
        return {};

    // []
    if (node->has_array_dims())
        type = apply_array_dims(type, node->array_dims());

    // &
    if (node->is_ref())
        type = type->ref_type();

    return type;
}

Ref<Type> Resolver::apply_array_dims(Ref<Type> type, Ref<ast::ExprList> dims) {
    return apply_array_dims(type, dims, Ref<ast::InitValue>{});
}

Ref<Type> Resolver::apply_array_dims(
    Ref<Type> type, Ref<ast::ExprList> dims, Ref<ast::InitValue> init) {
    assert(type);
    assert(dims && dims->child_num() > 0);
    assert(!dims->has_empty() || init);

    // get dimension list
    ArrayDimList dim_list;
    if (dims->has_empty()) {
        if (!init) {
            diag().error(
                dims, "array size must be set explicitly unless initializer "
                      "list is provided");
            return {};
        }
        bool ok = false;
        std::tie(dim_list, ok) = array_dims(dims->child_num(), init);
        if (!ok)
            return {};
    }

    // make array type
    for (unsigned n = 0; n < dims->child_num(); ++n) {
        auto expr = dims->get(n);
        array_size_t size{};
        if (expr) {
            // explicit size expr
            size = array_size(expr);
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

array_size_t Resolver::array_size(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n" << line_at(expr);

    // consteval
    auto fr = env().add_flags_raii(evl::Consteval);

    ExprRes res = env().eval_expr(expr);
    if (!res)
        return UnknownArraySize;

    // cast to Int
    auto int_type = builtins().int_type();
    res = env().cast(expr, int_type, std::move(res));
    if (!res)
        return UnknownArraySize;

    auto rval = res.move_value().move_rvalue();
    if (rval.empty())
        return UnknownArraySize;
    assert(rval.is_consteval());

    auto int_val = rval.get<Integer>();
    if (int_val < 0) {
        diag().error(expr, "array index is < 0");
        return UnknownArraySize;
    }
    return (array_size_t)int_val;
}

std::pair<ArrayDimList, bool>
Resolver::array_dims(unsigned num, Ref<ast::InitValue> init) {
    assert(num > 0);
    ArrayDimList dims;
    bool ok = true;

    if (!init->is<ast::InitList>()) {
        diag().error(init, "init value is not an array");
        return {std::move(dims), false};
    }

    // get array dimensions
    auto cur = init->get<ast::InitList>();
    while (cur) {
        dims.push_back(cur->size());
        if (dims.size() == num)
            break; // done

        auto& first = cur->get(0);
        first.accept(
            [&](Ptr<ast::InitList>& sublist) { cur = ref(sublist); },
            [&](auto&& other) {
                diag().error(ref(other), "not an array");
                cur = {};
                ok = false;
            });
    }
    return {std::move(dims), ok};
}

std::pair<TypedValueList, bool>
Resolver::eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl) {
    debug() << __FUNCTION__ << "\n" << line_at(args);
    assert(args);

    std::pair<TypedValueList, bool> res;
    auto fr = env().add_flags_raii(evl::Consteval);

    // eval args
    ExprResList arg_res_list;
    std::tie(arg_res_list, res.second) = eval_args(args);
    if (!res.second)
        return res;

    // param types and default values (if needed) are resolved in tpl scope
    auto scope_view = tpl->scope()->view(0);
    BasicScope inh_scope{&scope_view};              // tmp inheritance scope
    PersScope param_scope{&inh_scope, scp::Params}; // tmp param scope

    // create tmp class params
    EvalEnv::VarDefaults var_defaults;
    std::list<Ptr<Var>> params; // tmp class params
    for (auto tpl_param : tpl->params()) {
        // make tmp class param
        params.push_back(make<Var>(
            tpl_param->type_node(), tpl_param->node(), Ref<Type>{},
            Value{RValue{}}, tpl_param->flags()));
        auto param = ref(params.back());
        assert(param->is_local());
        param_scope.set(param->name_id(), param);

        // if arg provided, set default value override
        if (!arg_res_list.empty()) {
            var_defaults[param] = arg_res_list.pop_front();
        } else if (!param->node()->has_init()) {
            diag().error(args->loc_id(), 1, "not enough arguments");
            return res;
        }
    }

    // switch to tmp param scope
    auto param_scope_view = param_scope.view(0);
    auto ssr = env().scope_switch_raii(&param_scope_view);
    // set provided param values as defaults
    auto vdr = env().var_defaults_raii(std::move(var_defaults));

    if (program()->scope_options().allow_access_before_def) {
        Scope::GetParams sgp;
        sgp.current = true;

        // resolve parents, copy constants and typedefs
        if (tpl->node()->has_parents()) {
            auto parent_list = tpl->node()->parents();
            for (unsigned n = 0; n < parent_list->child_num(); ++n) {
                auto type_name = parent_list->get(n);
                auto parent = resolve_class_name(type_name);
                if (!parent)
                    return res;
                // typedefs
                for (auto alias : parent->type_defs()) {
                    auto name_id = alias->name_id();
                    if (!inh_scope.has(name_id, sgp))
                        inh_scope.set(name_id, alias);
                }
                // consts
                for (auto var : parent->consts()) {
                    auto name_id = var->name_id();
                    if (!inh_scope.has(name_id, sgp))
                        inh_scope.set(name_id, var);
                }
            }
        }

        // add copies of template constants and typedefs
        for (const auto& member : tpl->ordered_members()) {
            member.accept(
                [&](Ref<AliasType> alias) {
                    // typedef
                    auto name_id = alias->name_id();
                    if (inh_scope.has(name_id, sgp))
                        return;
                    auto copy = make<AliasType>(
                        program()->str_pool(), program()->builtins(),
                        &program()->type_id_gen(), alias->node());
                    inh_scope.set(name_id, std::move(copy));
                },
                [&](Ref<Var> var) {
                    // const
                    auto name_id = var->name_id();
                    if (inh_scope.has(name_id, sgp))
                        return;
                    auto copy = make<Var>(
                        var->type_node(), var->node(), Ref<Type>{}, Var::Const);
                    inh_scope.set(name_id, std::move(copy));
                },
                [&](auto&& other) {});
        }
    }

    // resolve params
    for (auto& param : params) {
        if (!resolve(ref(param)))
            return res;
        param_scope_view.advance();
    }

    // move param values into result
    for (auto& param : params)
        res.first.emplace_back(param->type(), param->move_value());
    res.second = true;
    return res;
}

std::pair<ExprResList, bool> Resolver::eval_args(Ref<ast::ArgList> args) {
    std::pair<ExprResList, bool> res;
    res.second = true;
    for (unsigned n = 0; res.second && n < args->child_num(); ++n) {
        auto arg_res = env().eval_expr(args->get(n));
        res.second = arg_res.ok();
        res.first.push_back(std::move(arg_res));
    }
    return res;
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

} // namespace ulam::sema

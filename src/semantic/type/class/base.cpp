#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class/base.hpp>

namespace ulam {

ClassBase::ClassBase(
    Ref<ast::ClassDef> node, Ref<Module> module, ScopeFlags scope_flags):
    _node{node},
    _module{module},
    _param_scope{make<PersScope>(module->scope())},
    _inh_scope{make<PersScope>(ref(_param_scope))},
    _scope{make<PersScope>(ref(_inh_scope), scope_flags)} {}

ClassKind ClassBase::kind() const { return node()->kind(); }

Ref<Var> ClassBase::add_param(Ref<ast::Param> node) {
    return add_param(node->type_name(), node);
}

Ref<Var>
ClassBase::add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto name_id = node->name_id();
    assert(!has(name_id));

    auto var =
        make<Var>(type_node, node, Ref<Type>{}, Var::ClassParam | Var::Const);
    auto ref = ulam::ref(var);
    var->set_scope_version(scope()->version());

    param_scope()->set(name_id, ref);
    set(name_id, std::move(var));
    _params.push_back(ref);

    if (!node->has_scope_version()) {
        node->set_var(ref);
        node->set_scope_version(param_scope()->version());
    }
    return ref;
}

Ref<AliasType> ClassBase::add_type_def(Ref<ast::TypeDef> node) {
    auto name_id = node->alias_id();
    auto& id_gen = module()->program()->type_id_gen();
    assert(!has(name_id));

    Ptr<UserType> type = make<AliasType>(&id_gen, node);
    auto ref = ulam::ref(type)->as_alias();
    type->set_scope_version(scope()->version());

    scope()->set(name_id, ref);
    set(name_id, std::move(type));

    if (!node->has_scope_version()) {
        node->set_alias_type(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<Fun> ClassBase::add_fun(Ref<ast::FunDef> node) {
    auto name_id = node->name_id();

    auto fun = make<Fun>(node);
    auto ref = ulam::ref(fun);
    fun->set_scope_version(scope()->version());

    auto params_node = fun->params_node();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        fun->add_param(param_node);
    }

    auto fset = add_fset(name_id);
    fset->add(std::move(fun));

    if (!node->has_scope_version()) {
        node->set_fun(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

void ClassBase::add_var_list(Ref<ast::VarDefList> node) {
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        if (node->is_const()) {
            add_const(node->type_name(), def);
        } else {
            add_prop(node->type_name(), def);
        }
    }
}

Ref<Var>
ClassBase::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto name_id = node->name_id();
    assert(!scope()->has(name_id));

    auto var = make<Var>(type_node, node, Ref<Type>{}, Var::Const);
    auto ref = ulam::ref(var);
    var->set_scope_version(scope()->version());

    scope()->set(name_id, ref);
    set(name_id, std::move(var));

    if (!node->has_scope_version()) {
        node->set_var(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<Prop>
ClassBase::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto name_id = node->name_id();
    assert(!scope()->has(name_id));

    auto prop = make<Prop>(type_node, node, Ref<Type>{}, Var::NoFlags);
    auto ref = ulam::ref(prop);
    prop->set_scope_version(scope()->version());

    scope()->set(name_id, ref);
    set(name_id, std::move(prop));
    _props.push_back(ref);

    if (!node->has_scope_version()) {
        node->set_prop(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<FunSet> ClassBase::add_fset(str_id_t name_id) {
    auto sym = get(name_id);
    assert(!sym || sym->is<FunSet>());
    if (!sym) {
        assert(_fsets.count(name_id) == 0);
        sym = set(name_id, make<FunSet>());
        scope()->set(name_id, sym->get<FunSet>());
        _fsets[name_id] = sym->get<FunSet>();
    }
    return sym->get<FunSet>();
}

} // namespace ulam

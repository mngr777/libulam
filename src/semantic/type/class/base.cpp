#include "libulam/semantic/fun.hpp"
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
    auto name_id = node->name_id();
    assert(!has(name_id));
    auto var = make<Var>(
        node->type_name(), node, Ref<Type>{}, Var::ClassParam | Var::Const);
    auto ref = ulam::ref(var);
    param_scope()->set(name_id, ref);
    if (!node->has_scope_version())
        node->set_scope_version(param_scope()->version());
    set(name_id, std::move(var));
    return ref;
}

Ref<AliasType> ClassBase::add_type_def(Ref<ast::TypeDef> node) {
    auto& id_gen = module()->program()->type_id_gen();
    auto name_id = node->alias_id();
    Ptr<UserType> type = make<AliasType>(&id_gen, node);
    auto ref = ulam::ref(type)->as_alias();
    scope()->set(name_id, ref);
    if (!node->has_scope_version())
        node->set_scope_version(scope()->version());
    set(name_id, std::move(type));
    return ref;
}

Ref<Fun> ClassBase::add_fun(Ref<ast::FunDef> node) {
    auto name_id = node->name_id();
    auto sym = get(name_id);
    assert(!sym || sym->is<FunSet>());
    if (!sym) {
        sym = set(name_id, make<FunSet>());
        scope()->set(name_id, sym->get<FunSet>());
    }
    auto fset = sym->get<FunSet>();

    auto fun = make<Fun>(node);
    auto ref = ulam::ref(fun);
    fun->set_scope_version(scope()->version());
    if (!node->has_scope_version())
        node->set_scope_version(scope()->version());

    auto params_node = fun->params_node();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        fun->add_param(param_node);
    }

    fset->add(std::move(fun));
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

Ref<Var> ClassBase::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = make<Var>(type_node, node, Ref<Type>{}, Var::Const);
    auto ref = ulam::ref(var);
    auto name_id = var->name_id();
    scope()->set(name_id, ref);
    set(name_id, std::move(var));
    if (!node->has_scope_version()) {
        node->set_var(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<Prop> ClassBase::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto prop = make<Prop>(type_node, node, Ref<Type>{}, Var::NoFlags);
    auto ref = ulam::ref(prop);
    auto name_id = prop->name_id();
    scope()->set(name_id, ref);
    set(name_id, std::move(prop));
    if (!node->has_scope_version()) {
        node->set_prop(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

} // namespace ulam

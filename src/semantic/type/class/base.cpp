#include "libulam/semantic/fun.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam {

ClassBase::ClassBase(
    Ref<ast::ClassDef> node, Ref<Module> module, ScopeFlags scope_flags):
    _node{node},
    _module{module},
    _param_scope{make<PersScope>(module->scope())},
    _inh_scope{make<PersScope>(ref(_param_scope))},
    _scope{make<PersScope>(ref(_inh_scope), scope_flags)} {
}

ClassKind ClassBase::kind() const { return node()->kind(); }

void ClassBase::add_param(Ref<ast::Param> node) {
    auto name_id = node->name().str_id();
    assert(!has(name_id));
    auto var = make<Var>(
        node->type_name(), node, Ref<Type>{}, Var::ClassParam | Var::Const);
    param_scope()->set(name_id, ref(var));
    node->set_scope_version(param_scope()->version());
    set(name_id, std::move(var));
}

void ClassBase::add_type_def(Ref<ast::TypeDef> node) {
    auto& id_gen = module()->program()->type_id_gen();
    auto name_id = node->alias_id();
    Ptr<UserType> type = make<AliasType>(&id_gen, node);
    scope()->set(name_id, ref(type));
    node->set_scope_version(scope()->version());
    set(name_id, std::move(type));
}

void ClassBase::add_fun(Ref<ast::FunDef> node) {
    auto name_id = node->name_id();

    auto sym = get(name_id);
    assert(!sym || sym->is<FunSet>());
    if (!sym) {
        sym = set(name_id, make<FunSet>());
        scope()->set(name_id, sym->get<FunSet>());
    }
    auto fset = sym->get<FunSet>();

    auto fun = make<Fun>(node);
    fun->set_scope_version(scope()->version());
    node->set_scope_version(scope()->version());

    auto params_node = fun->params_node();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        fun->add_param(param_node);
    }

    fset->add(std::move(fun));
}

} // namespace ulam

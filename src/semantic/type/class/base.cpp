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
    _inh_scope{make<PersScope>(module->scope())},
    _param_scope{make<PersScope>(ref(_param_scope))},
    _scope{make<PersScope>(ref(_inh_scope), scope_flags)} {}

ClassKind ClassBase::kind() const { return node()->kind(); }

bool ClassBase::has_op(Op op) const {
    return _ops.count(op) > 0;
}

Ref<FunSet> ClassBase::op(Op op) {
    auto it = _ops.find(op);
    return (it != _ops.end()) ? ref(it->second) : Ref<FunSet>{};
}

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
    auto program = module()->program();
    assert(!has(name_id));

    Ptr<UserType> type =
        make<AliasType>(program->builtins(), &program->type_id_gen(), node);
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

    auto fset = node->is_op() ? find_fset(node->op()) : find_fset(name_id);
    fset->add(std::move(fun));

    if (!node->has_scope_version()) {
        node->set_fun(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
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

    if (!node->has_scope_version()) {
        node->set_prop(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<FunSet> ClassBase::find_fset(str_id_t name_id) {
    auto sym = get(name_id);
    assert(!sym || sym->is<FunSet>());
    return sym ? sym->get<FunSet>() : add_fset(name_id);
}

Ref<FunSet> ClassBase::find_fset(Op op) {
    auto it = _ops.find(op);
    return (it != _ops.end()) ? ref(it->second) : add_fset(op);
}

Ref<FunSet> ClassBase::add_fset(str_id_t name_id) {
    auto fset = make<FunSet>();
    auto ref = ulam::ref(fset);
    set(name_id, std::move(fset));
    scope()->set(name_id, ref);
    return ref;
}

Ref<FunSet> ClassBase::add_fset(Op op) {
    assert(_ops.count(op) == 0);
    auto fset = make<FunSet>();
    auto ref = ulam::ref(fset);
    _ops[op] = std::move(fset);
    return ref;
}

} // namespace ulam

#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

ClassBase::ClassBase(
    Ref<ast::ClassDef> node, Ref<Module> module, scope_flags_t scope_flags):
    _node{node},
    _module{module},
    _inh_scope{make<BasicScope>(module->scope())},
    _param_scope{make<PersScope>(ref(_inh_scope))},
    _scope{make<PersScope>(ref(_param_scope), scope_flags)},
    _constructors(make<FunSet>()) {}

ClassKind ClassBase::kind() const { return node()->kind(); }

bool ClassBase::has(str_id_t name_id) const { return _members.has(name_id); }

bool ClassBase::has_fun(str_id_t name_id) const {
    auto sym = get(name_id);
    return sym && sym->is<FunSet>();
}

bool ClassBase::has_fun(const std::string_view name) const {
    auto sym = get(name);
    return sym && sym->is<FunSet>();
}

Ref<FunSet> ClassBase::fun(str_id_t name_id) {
    assert(has_fun(name_id));
    auto sym = get(name_id);
    return sym->get<FunSet>();
}

Ref<FunSet> ClassBase::fun(const std::string_view name) {
    assert(has_fun(name));
    auto sym = get(name);
    return sym->get<FunSet>();
}

ClassBase::Symbol* ClassBase::get(const std::string_view name) {
    return const_cast<Symbol*>(const_cast<const ClassBase*>(this)->get(name));
}

const ClassBase::Symbol* ClassBase::get(const std::string_view name) const {
    auto name_id = _module->program()->str_pool().id(name);
    if (name_id == NoStrId)
        return {};
    return get(name_id);
}

ClassBase::Symbol* ClassBase::get(str_id_t name_id) {
    return _members.get(name_id);
}

const ClassBase::Symbol* ClassBase::get(str_id_t name_id) const {
    return _members.get(name_id);
}

bool ClassBase::has_op(Op op) const { return _ops.count(op) > 0; }

Ref<FunSet> ClassBase::op(Op op) {
    auto it = _ops.find(op);
    return (it != _ops.end()) ? ref(it->second) : Ref<FunSet>{};
}

Ref<Var> ClassBase::add_param(Ptr<Var>&& var) {
    auto ref = ulam::ref(var);
    auto name_id = ref->name_id();
    assert(!has(name_id));

    param_scope()->set(name_id, std::move(var));
    set(name_id, ref);
    _params.push_back(ref);

    auto node = ref->node();
    if (!node->has_scope_version()) {
        node->set_var(ref);
        node->set_scope_version(param_scope()->version());
    }
    return ref;
}

Ref<AliasType> ClassBase::add_type_def(Ref<ast::TypeDef> node) {
    auto name_id = node->alias_id();
    auto program = _module->program();
    assert(!has(name_id));

    Ptr<UserType> type = make<AliasType>(
        program->str_pool(), program->builtins(), &program->type_id_gen(),
        node);

    auto ref = ulam::ref(type)->as_alias();
    scope()->set(name_id, std::move(type));
    set(name_id, ref);

    if (!node->has_scope_version())
        node->set_scope_version(scope()->version());
    return ref;
}

Ref<Fun> ClassBase::add_fun(Ref<ast::FunDef> node) {
    auto name_id = node->name_id();

    auto fun = make<Fun>(_module->program()->mangler(), scope(), node);
    auto ref = ulam::ref(fun);
    fun->set_scope_version(scope()->version());

    auto params_node = fun->params_node();
    for (unsigned n = 0; n < params_node->child_num(); ++n) {
        auto param_node = params_node->get(n);
        fun->add_param(param_node);
    }

    if (node->is_constructor()) {
        constructors()->add(std::move(fun));
    } else {
        if (!node->is_op() || node->is_op_alias()) {
            if (node->is_op())
                find_op_fset(node->op())->add(ref);
            find_fset(name_id)->add(std::move(fun));
        } else {
            find_op_fset(node->op())->add(std::move(fun));
        }
    }

    if (!node->has_scope_version()) {
        node->set_fun(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

Ref<Var>
ClassBase::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto name_id = node->name_id();
    assert(!scope()->has(name_id, true));

    auto var = make<Var>(type_node, node, Ref<Type>{}, Var::Const);
    auto ref = ulam::ref(var);

    scope()->set(name_id, std::move(var));
    set(name_id, ref);

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

    scope()->set(name_id, std::move(prop));
    set(name_id, ref);

    if (!node->has_scope_version()) {
        node->set_prop(ref);
        node->set_scope_version(scope()->version());
    }
    return ref;
}

bool ClassBase::has_constructors() const { return (bool)_constructors; }

Ref<FunSet> ClassBase::constructors() {
    assert(_constructors);
    return ref(_constructors);
}

Ref<FunSet> ClassBase::find_fset(str_id_t name_id) {
    auto sym = get(name_id);
    assert(!sym || sym->is<FunSet>());
    return sym ? sym->get<FunSet>() : add_fset(name_id);
}

Ref<FunSet> ClassBase::find_op_fset(Op op) {
    auto it = _ops.find(op);
    return (it != _ops.end()) ? ref(it->second) : add_op_fset(op);
}

Ref<FunSet> ClassBase::add_fset(str_id_t name_id) {
    auto fset = make<FunSet>(name_id);
    auto ref = ulam::ref(fset);
    scope()->set(name_id, std::move(fset));
    set(name_id, ref);
    return ref;
}

Ref<FunSet> ClassBase::add_op_fset(Op op) {
    assert(_ops.count(op) == 0);
    auto fset = make<FunSet>();
    auto ref = ulam::ref(fset);
    _ops[op] = std::move(fset);
    return ref;
}

} // namespace ulam

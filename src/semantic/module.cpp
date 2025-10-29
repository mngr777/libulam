#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/iter.hpp>

namespace ulam {

Module::Module(Ref<Program> program, Ref<ast::ModuleDef> node):
    _program{program},
    _node{node},
    _env_scope{make<BasicScope>(nullptr, scp::ModuleEnv)},
    _scope{make<ModuleScope>(this, ref(_env_scope), scp::Module)} {}

Module::~Module() {}

str_id_t Module::name_id() const { return node()->name_id(); }

const std::string_view Module::name() const {
    return _program->str_pool().get(name_id());
}

version_t Module::ulam_version() const { return node()->ulam_version(); }

Ref<AliasType> Module::add_type_def(Ref<ast::TypeDef> node) {
    auto name_id = node->alias_id();
    Ptr<UserType> type = make<AliasType>(
        program()->str_pool(), program()->builtins(), &program()->type_id_gen(),
        node);
    auto ref = ulam::ref(type)->as_alias();
    type->set_module(this);
    scope()->set(name_id, std::move(type));

    assert(!node->has_scope_version());
    node->set_scope_version(scope()->version());
    return ref;
}

void Module::add_const_list(Ref<ast::VarDefList> node) {
    for (unsigned n = 0; n < node->def_num(); ++n)
        add_const(node->type_name(), node->def(n));
}

Ref<Var>
Module::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDef> node) {
    auto var = make<Var>(type_node, node, Ref<Type>{}, Var::Const);
    auto ref = ulam::ref(var);
    var->set_module(this);
    scope()->set(ref->name_id(), std::move(var));

    assert(!node->has_scope_version());
    node->set_var(ref);
    node->set_scope_version(scope()->version());
    return ref;
}

Ref<Class> Module::add_class(Ref<ast::ClassDef> node) {
    auto name = program()->str_pool().get(node->name_id());
    auto cls = make<Class>(name, node, this);
    auto ref = ulam::ref(cls);
    auto name_id = ref->name_id();

    scope()->set(name_id, std::move(cls));
    _env_scope->set(name_id, ref);
    auto sym = set(name_id, ref);
    add_export(node, name_id, sym);

    assert(!node->has_scope_version());
    node->set_cls(ref);
    node->set_scope_version(scope()->version());

    _classes.push_back(ref);
    return ref;
}

Ref<ClassTpl> Module::add_class_tpl(Ref<ast::ClassDef> node) {
    assert(node->params()->child_num() > 0);
    const auto& str_pool = program()->str_pool();
    auto& diag = program()->diag();

    auto name = str_pool.get(node->name_id());
    auto tpl = make<ClassTpl>(name, node, this);
    auto ref = ulam::ref(tpl);
    auto name_id = ref->name_id();

    auto params = node->params();
    for (unsigned n = 0; n < params->child_num(); ++n) {
        auto param = params->get(n);
        if (ref->has(param->name_id()))
            diag.error(param, "already defined");
        ref->add_param(param);
    }

    scope()->set(name_id, std::move(tpl));
    _env_scope->set(name_id, ref);
    auto sym = set(name_id, ref);
    add_export(node, name_id, sym);

    assert(!node->has_scope_version());
    node->set_cls_tpl(ref);
    node->set_scope_version(scope()->version());

    _class_tpls.push_back(ref);
    return ref;
}

void Module::export_symbols(Scope* scope) {
    for (auto& pair : _symbols) {
        auto& [name_id, sym] = pair;
        if (sym.is<Class>()) {
            scope->set<UserType>(name_id, sym.get<Class>());
        } else {
            assert(sym.is<ClassTpl>());
            scope->set<ClassTpl>(name_id, sym.get<ClassTpl>());
        }
    }
}

Module::Symbol* Module::get(const std::string_view name) {
    return const_cast<Module::Symbol*>(
        const_cast<const Module*>(this)->get(name));
}

const Module::Symbol* Module::get(const std::string_view name) const {
    auto name_id = _program->str_pool().id(name);
    if (name_id == NoStrId)
        return {};
    return get(name_id);
}

Module::Symbol* Module::get(str_id_t name_id) { return _symbols.get(name_id); }

const Module::Symbol* Module::get(str_id_t name_id) const {
    return _symbols.get(name_id);
}

bool Module::resolve(sema::Resolver& resolver) {
    bool ok = true;
    for (auto [_, sym] : *scope()) {
        bool resolved = sym->accept(
            [&](Ref<UserType> type) {
                if (type->is_alias()) {
                    assert(type->is_alias());
                    auto scope_version = type->as_alias()->scope_version();
                    auto scope_view = scope()->view(scope_version);
                    return resolver.resolve(type->as_alias()) && ok;
                } else {
                    return resolver.init(type->as_class());
                }
            },
            [&](Ref<ClassTpl> tpl) { return true; },
            [&](Ref<Var> var) {
                auto scope_view = scope()->view(var->scope_version());
                return resolver.resolve(var);
            },
            [&](auto) -> bool { assert(false); });
        ok = ok && resolved;
    }
    return ok;
}

void Module::add_export(Ref<ast::Node> node, str_id_t name_id, Symbol* sym) {
    auto& str_pool = program()->str_pool();
    auto& diag = program()->diag();

    auto existing = program()->add_export(name_id, {this, sym});
    if (existing) {
        assert(existing->module() != this);
        auto other_module_name = str_pool.get(existing->module()->name_id());
        auto message =
            "name conflicts with name in " + std::string{other_module_name};
        diag.error(node, message);
    }
}

} // namespace ulam

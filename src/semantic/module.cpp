#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Module::Module(Ref<Program> program, module_id_t id, Ref<ast::ModuleDef> node):
    _program{program},
    _id{id},
    _node{node},
    _env_scope{make<PersScope>(Ref<Scope>{}, scp::ModuleEnv)},
    _scope{make<PersScope>(ref(_env_scope), scp::Module)} {}

Module::~Module() {}

void Module::add_type_def(Ref<ast::TypeDef> node) {
    auto name_id = node->alias_id();
    Ptr<UserType> type = make<AliasType>(&program()->type_id_gen(), node);
    scope()->set(name_id, std::move(type)); // ??
    node->set_scope_version(scope()->version());
}

void Module::add_class_or_tpl(Ref<ast::ClassDef> node) {
    assert(!node->cls() && !node->cls_tpl());
    if (node->has_params()) {
        add_class_tpl(node);
    } else {
        add_class(node);
    }
}

void Module::add_class(Ref<ast::ClassDef> node) {
    auto name = program()->str_pool().get(node->name_id());
    auto cls = make<Class>(name, node, this);
    scope()->set(cls->name_id(), ref(cls));
    cls->node()->set_scope_version(scope()->version());
    set(cls->name_id(), std::move(cls));
}

void Module::add_class_tpl(Ref<ast::ClassDef> node) {
    assert(node->params()->child_num() > 0);
    assert(node->kind() != ClassKind::Element);

    auto name_id = node->name_id();
    auto tpl = make<ClassTpl>(node, this);

    auto params = node->params();
    for (unsigned n = 0; n < params->child_num(); ++n) {
        auto param = params->get(n);
        if (tpl->has(param->name_id()))
            program()->diag().error(param, "already defined");
        tpl->add_param(param);
    }

    scope()->set(name_id, ref(tpl));
    node->set_scope_version(scope()->version());
    set(tpl->name_id(), std::move(tpl));
}

void Module::export_symbols(Ref<Scope> scope) {
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

void Module::add_import(str_id_t name_id, Ref<Module> module, Ref<Class> type) {
    assert(_imports.count(name_id) == 0);
    assert(module != this);
    _imports.emplace(name_id, Import{module, type});
    _env_scope->set(name_id, type);
}

void Module::add_import(
    str_id_t name_id, Ref<Module> module, Ref<ClassTpl> type_tpl) {
    assert(_imports.count(name_id) == 0);
    assert(module != this);
    _imports.emplace(name_id, Import{module, type_tpl});
    _env_scope->set(name_id, type_tpl);
}

} // namespace ulam

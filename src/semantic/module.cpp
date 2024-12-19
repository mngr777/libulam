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

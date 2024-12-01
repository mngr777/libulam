#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Module::Module(
    Ref<Program> program, module_id_t id, ast::Ref<ast::ModuleDef> node):
    _program{program},
    _id{id},
    _node{node},
    _scope{make<Scope>(Ref<Scope>{}, Scope::Module)} {}

Module::~Module() {}

void Module::export_imports(Ref<Scope> scope) {
    for (auto pair : _imports) {
        auto& [name_id, import] = pair;
        assert(!scope->has(name_id));
        scope->set(name_id, import.type());
    }
}

void Module::add_import(str_id_t name_id, Ref<Module> module, Ref<Type> type) {
    assert(_imports.count(name_id) == 0);
    assert(module != this);
    _imports.emplace(name_id, Import{module, type});
}

} // namespace ulam

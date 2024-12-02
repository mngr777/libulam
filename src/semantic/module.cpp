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
    _env_scope{make<Scope>(this, program->scope(), Scope::ModuleEnv)},
    _scope{make<Scope>(ref(_env_scope), Scope::Module)} {}

Module::~Module() {}

void Module::add_import(str_id_t name_id, Ref<Module> module, Ref<Type> type) {
    assert(_imports.count(name_id) == 0);
    assert(module != this);
    _imports.emplace(name_id, Import{module, type});
    _env_scope->set(name_id, type);
}

void Module::add_import(
    str_id_t name_id, Ref<Module> module, Ref<TypeTpl> type_tpl) {
    assert(_imports.count(name_id) == 0);
    assert(module != this);
    _imports.emplace(name_id, Import{module, type_tpl});
    _env_scope->set(name_id, type_tpl);
}

} // namespace ulam

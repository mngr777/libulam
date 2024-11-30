#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Module::Module(Ref<Program> program, module_id_t id, ast::Ref<ast::ModuleDef> node):
    _program{program},
    _id{id},
    _node{node},
    _scope{make<Scope>(Ref<Scope>{}, Scope::Module)} {}

Module::~Module() {}

} // namespace ulam

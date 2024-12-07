#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, ast::Ref<ast::Root> ast):
    _diag{diag}, _ast{ast}, _builtins{_type_id_gen} {}

Program::~Program() {}

str_id_t Program::self_str_id() { return _ast->ctx().str_id("Self"); }

str_id_t Program::self_inst_str_id() { return _ast->ctx().str_id("self"); }

Ref<Module> Program::module(module_id_t id) {
    assert(id < _modules.size());
    return ref(_modules[id]);
}

Ref<Module> Program::add_module(ast::Ref<ast::ModuleDef> node) {
    module_id_t id = _modules.size();
    _modules.push_back(ulam::make<Module>(this, id, node));
    return ref(_modules[id]);
}

} // namespace ulam

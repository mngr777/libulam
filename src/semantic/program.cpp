#include <cassert>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, Ref<ast::Root> ast):
    _diag{diag}, _ast{ast}, _builtins{_type_id_gen} {}

Program::~Program() {}

Ref<Module> Program::module(module_id_t id) {
    assert(id < _modules.size());
    return ref(_modules[id]);
}

Ref<Module> Program::add_module(Ref<ast::ModuleDef> node) {
    module_id_t id = _modules.size();
    _modules.push_back(make<Module>(this, id, node));
    return ref(_modules[id]);
}

} // namespace ulam

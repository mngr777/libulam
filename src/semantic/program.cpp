#include "libulam/str_pool.hpp"
#include <cassert>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, UniqStrPool& str_pool):
    _diag{diag}, _str_pool{str_pool}, _builtins{_type_id_gen} {}

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

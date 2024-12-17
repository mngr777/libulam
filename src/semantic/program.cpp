#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, Ref<ast::Root> ast):
    _diag{diag}, _ast{ast}, _builtins{_type_id_gen} {}

Program::~Program() {}

str_id_t Program::self_str_id() { return _ast->ctx().str_id("Self"); }

str_id_t Program::self_inst_str_id() { return _ast->ctx().str_id("self"); }

Ref<Module> Program::module(module_id_t id) {
    assert(id < _modules.size());
    return ref(_modules[id]);
}

Ref<Module> Program::module(const std::string& name) {
    auto name_id = _ast->ctx().str_pool().id(name);
    if (name_id == NoStrId)
        return {};
    auto it = _modules_by_name_id.find(name_id);
    return (it != _modules_by_name_id.end()) ? it->second : Ref<Module>{};
}

Ref<Module> Program::add_module(Ref<ast::ModuleDef> node) {
    auto name_id = node->name().str_id();
    assert(_modules_by_name_id.count(name_id) == 0);

    module_id_t id = _modules.size();
    _modules.push_back(make<Module>(this, id, node));
    _modules_by_name_id[name_id] = ref(_modules[id]);
    return ref(_modules[id]);
}

} // namespace ulam

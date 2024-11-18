#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <utility>

namespace ulam {

Program::Program(ast::Ref<ast::Root> ast):
    _ast{ast}, _scope{make<Scope>(Ref<Scope>{}, Scope::Program)} {}

void Program::add_module(Ptr<Module>&& module) {
    _modules.push_back(std::move(module));
}

} // namespace ulam

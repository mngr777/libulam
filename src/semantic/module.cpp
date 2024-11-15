#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Module::Module(ast::Ref<ast::ModuleDef> node):
    _node{node}, _scope{make<Scope>(Ref<Scope>{}, Scope::Module)} {}

} // namespace ulam

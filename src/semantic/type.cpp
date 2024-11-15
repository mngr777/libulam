#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

Class::Class(Ref<ast::ClassDef> node):
    _node{node}, _scope{make<Scope>(Ref<Scope>{}, Scope::Class)} {}

Class::~Class() {}

} // namespace ulam

#include <libulam/semantic/class.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BaseType{id},
    _node{node},
    _scope{make<Scope>(Ref<Scope>{}, Scope::Class)} {}

Class::~Class() {}

} // namespace ulam

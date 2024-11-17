#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BasicType{id},
    _node{node},
    _scope{make<Scope>(Ref<Scope>{}, Scope::Class)} {}

Class::~Class() {}

} // namespace ulam

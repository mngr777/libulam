#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BasicType{id},
    _node{node} {}

Class::~Class() {}

} // namespace ulam

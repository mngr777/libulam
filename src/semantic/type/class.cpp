#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BasicType{id}, _node{node} {}

Class::~Class() {}

Ref<Type> ClassTpl::type(
    Diag& diag, TypeIdGen& type_id_gen, ast::Ref<ast::ArgList> args) {
    return {};
}

} // namespace ulam

#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BasicType{id}, _node{node} {}

Class::~Class() {}

Ref<Type> ClassTpl::type(
    Diag& diag,
    ast::Ref<ast::ArgList> arg_list,
    TypeIdGen& type_id_gen,
    ValueList& args) {
    return {};
}

} // namespace ulam

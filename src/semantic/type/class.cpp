#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

// Class

Class::Class(type_id_t id, Ref<ast::ClassDef> node):
    BasicType{id}, _node{node} {}

Class::~Class() {}

void Class::export_symbols(Scope* scope) { scope->import_symbols(_members); }

// ClassTpl

Ref<Type> ClassTpl::type(ast::Ref<ast::ArgList> arg_list, ValueList& args) {
    return {};
}

void ClassTpl::export_symbols(Scope* scope) { scope->import_symbols(_members); }

} // namespace ulam

#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <utility>

namespace ulam {

Module::Module(ast::Ref<ast::ModuleDef> node): _node{node} {}

void Module::export_symbols(Scope* scope) {
    scope->import_symbols(_symbols);
}

void Module::add_import(ast::Ref<ast::TypeSpec> node) {
    auto str_id = node->ident()->name().str_id();
    auto [it, inserted] = _imports.emplace(str_id, Import{});
    it->second.add_node(node);
}

} // namespace ulam

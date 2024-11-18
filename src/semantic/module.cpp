#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

Module::Module(ast::Ref<ast::ModuleDef> node):
    _node{node}, _scope{make<Scope>(Ref<Scope>{}, Scope::Module)} {}

void Module::add_export(ast::Ref<ast::ClassDef> node) {
    auto str_id = node->name().str_id();
    assert(_exports.count(str_id) == 0);
    _exports.emplace(str_id, Export{node});
}

void Module::add_import(ast::Ref<ast::TypeSpec> node) {
    auto str_id = node->ident()->name().str_id();
    auto [it, inserted] = _imports.emplace(str_id, Import{});
    it->second.add_node(node);
}

} // namespace ulam

#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <utility>

namespace ulam {

Module::Module(ast::Ref<ast::ModuleDef> node): _node{node} {}

void Module::export_symbols(Scope* scope) {
    for (auto& pair : _symbols) {
        auto& [name_id, sym] = pair;
        if (sym.is<Type>() && sym.get<Type>()->basic()->is_class()) {
            assert(sym.get<Type>()->is_basic());
            scope->set(name_id, sym.get<Type>());
        }
    }
}

void Module::add_import(ast::Ref<ast::TypeSpec> node) {
    auto str_id = node->ident()->name().str_id();
    auto [it, inserted] = _imports.emplace(str_id, Import{});
    it->second.add_node(node);
}

} // namespace ulam

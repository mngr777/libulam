#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <utility>

namespace ulam {

Module::Module(module_id_t id, ast::Ref<ast::ModuleDef> node): _id{id}, _node{node} {}

void Module::export_symbols(Scope* scope) {
    for (auto& pair : _symbols) {
        auto& [name_id, sym] = pair;
        if (sym.is<Type>() && sym.get<Type>()->basic()->is_class()) {
            assert(sym.get<Type>()->is_basic());
            scope->set(name_id, sym.get<Type>());
        }
    }
}

} // namespace ulam

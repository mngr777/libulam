#include <cassert>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <utility>

namespace ulam {

Module::Module(ast::Ref<ast::ModuleDef> node): _node{node} {}

void Module::export_classes(Scope* scope) {
    for (auto& pair : _classes)
        scope->set(pair.first, ref(pair.second));
}

bool Module::has_class(str_id_t name_id) { return get_class(name_id); }

Ref<Class> Module::get_class(str_id_t name_id) {
    auto it = _classes.find(name_id);
    return (it != _classes.end()) ? ref(it->second) : Ref<Class>{};
}

void Module::add_class(Ptr<Class>&& cls) {
    auto name_id = cls->node()->name().str_id();
    assert(_classes.count(name_id) == 0);
    _classes.emplace(name_id, std::move(cls));
}

void Module::add_import(ast::Ref<ast::TypeSpec> node) {
    auto str_id = node->ident()->name().str_id();
    auto [it, inserted] = _imports.emplace(str_id, Import{});
    it->second.add_node(node);
}

} // namespace ulam

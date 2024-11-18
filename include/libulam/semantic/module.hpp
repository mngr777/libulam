#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/export_import.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam::ast {
class ModuleDef;
}

namespace ulam {

class Scope;

class Module {
public:
    Module(ast::Ref<ast::ModuleDef> node);

    Ref<Scope> scope() { return ref(_scope); }

    auto& exports() { return _exports; }
    const auto& exports() const { return _exports; }

    void add_export(ast::Ref<ast::ClassDef> node);

    auto& imports() { return _imports; }
    const auto& imports() const { return _imports; }

    void add_import(ast::Ref<ast::TypeSpec> node);

private:
    ast::Ref<ast::ModuleDef> _node;
    Ptr<Scope> _scope;
    std::unordered_map<str_id_t, Export> _exports;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam

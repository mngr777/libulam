#pragma once
#include "libulam/ast/nodes/module.hpp"
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/export_import.hpp>
#include <list>

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

    void add_export(Export&& exp) { _exports.push_back(std::move(exp)); }

    auto& imports() { return _imports; }
    const auto& imports() const { return _imports; }

    void add_import(Import&& imp) { _imports.push_back(std::move(imp)); }

private:
    ast::Ref<ast::ModuleDef> _node;
    Ptr<Scope> _scope;
    std::list<Export> _exports;
    std::list<Import> _imports;
};

} // namespace ulam

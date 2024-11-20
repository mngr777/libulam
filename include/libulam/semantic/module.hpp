#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/import.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam::ast {
class ModuleDef;
class TypeSpec;
} // namespace ulam::ast

namespace ulam {

class Scope;

class Module {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl>;
    using Symbol = SymbolTable::Symbol;

    Module(ast::Ref<ast::ModuleDef> node);

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    void export_symbols(Scope* scope);

    Symbol* get(str_id_t name_id) {
        return _symbols.get(name_id);
    }

    template <typename T>
    void set(str_id_t name_id, Ptr<T>&& value) {
        _symbols.set(name_id, std::move(value));
    }

    auto& imports() { return _imports; }
    const auto& imports() const { return _imports; }

    void add_import(ast::Ref<ast::TypeSpec> node);

private:
    ast::Ref<ast::ModuleDef> _node;
    SymbolTable _symbols;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam

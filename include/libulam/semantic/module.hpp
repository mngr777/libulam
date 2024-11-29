#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {
class ModuleDef;
class TypeSpec;
} // namespace ulam::ast

namespace ulam {

using module_id_t = std::uint16_t;
constexpr module_id_t NoModuleId = 0;

class Scope;

class Module {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl>;
    using Symbol = SymbolTable::Symbol;

    Module(module_id_t id, ast::Ref<ast::ModuleDef> node);

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    module_id_t id() const { return _id; }

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

private:
    module_id_t _id;
    ast::Ref<ast::ModuleDef> _node;
    SymbolTable _symbols;
};

} // namespace ulam

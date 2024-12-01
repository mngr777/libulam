#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <set>
#include <unordered_map>

namespace ulam::ast {
class ModuleDef;
class TypeSpec;
} // namespace ulam::ast

namespace ulam {

class Program;

using module_id_t = std::uint16_t;
constexpr module_id_t NoModuleId = 0;

class Module;

class Import {
public:
    Import(Ref<Module> module, Ref<Type> type):
        _module{module}, _type{type}, _type_tpl{} {}

    Import(Ref<Module> module, Ref<TypeTpl> type_tpl):
        _module{module}, _type{}, _type_tpl{type_tpl} {}

    Ref<Module> module() { return _module; }
    Ref<Type> type() { return _type; }
    Ref<TypeTpl> type_tpl() { return _type_tpl; }

private:
    Ref<Module> _module;
    Ref<Type> _type;
    Ref<TypeTpl> _type_tpl;
};

class Scope;

class Module {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl>;
    using Symbol = SymbolTable::Symbol;

    Module(Ref<Program> program, module_id_t id, ast::Ref<ast::ModuleDef> node);
    ~Module();

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    Ref<Program> program() { return _program; }

    module_id_t id() const { return _id; }

    ast::Ref<ast::ModuleDef> node() { return _node; }

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    Ref<Scope> scope() { return ref(_scope); }

    Symbol* get(str_id_t name_id) { return _symbols.get(name_id); }

    template <typename T> void set(str_id_t name_id, Ptr<T>&& value) {
        _symbols.set(name_id, std::move(value));
    }

    const auto& deps() const { return _deps; }
    void add_dep(str_id_t name_id) { _deps.insert(name_id); }

    // NOTE: scope is supposed to be empty at this point
    void export_imports(Ref<Scope> scope);
    void add_import(str_id_t name_id, Ref<Module> module, Ref<Type> type);
    void add_import(str_id_t name_id, Ref<Module> module, Ref<TypeTpl> type_tpl);

private:
    Ref<Program> _program;
    module_id_t _id;
    ast::Ref<ast::ModuleDef> _node;
    Ptr<Scope> _scope;
    SymbolTable _symbols;
    std::set<str_id_t> _deps;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam

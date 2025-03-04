#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/str_pool.hpp>
#include <set>
#include <unordered_map>

namespace ulam::ast {
class ModuleDef;
class TypeName;
class TypeSpec;
class VarDef;
class VarDefList;
} // namespace ulam::ast

namespace ulam::sema {
class Resolver;
}

namespace ulam {

class Program;

using module_id_t = std::uint16_t;
constexpr module_id_t NoModuleId = 0;

class Module;

class Import {
public:
    Import(Ref<Module> module, Ref<Class> type):
        _module{module}, _type{type}, _type_tpl{} {}

    Import(Ref<Module> module, Ref<ClassTpl> type_tpl):
        _module{module}, _type{}, _type_tpl{type_tpl} {}

    Ref<Module> module() { return _module; }
    Ref<Class> type() { return _type; }
    Ref<ClassTpl> type_tpl() { return _type_tpl; }

private:
    Ref<Module> _module;
    Ref<Class> _type;
    Ref<ClassTpl> _type_tpl;
};

class PersScope;

class Module {
public:
    using SymbolTable = _SymbolTable<Class, ClassTpl>;
    using Symbol = SymbolTable::Symbol;

    Module(Ref<Program> program, module_id_t id, Ref<ast::ModuleDef> node);
    ~Module();

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    bool operator==(const Module& other) const { return id() == other.id(); }
    bool operator!=(const Module& other) const { return !operator==(other); }

    Ref<Program> program() { return _program; }

    module_id_t id() const { return _id; }

    Ref<ast::ModuleDef> node() { return _node; }

    Ref<AliasType> add_type_def(Ref<ast::TypeDef> node);
    void add_const_list(Ref<ast::VarDefList> node);
    Ref<Var> add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDef> node);
    Ref<Class> add_class(Ref<ast::ClassDef> node);
    Ref<ClassTpl> add_class_tpl(Ref<ast::ClassDef> node);

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    void export_symbols(Ref<Scope> scope);

    Ref<PersScope> scope() { return ref(_scope); }

    Symbol* get(str_id_t name_id) { return _symbols.get(name_id); }

    template <typename T> void set(str_id_t name_id, Ptr<T>&& value) {
        _symbols.set(name_id, std::move(value));
    }

    const auto& deps() const { return _deps; }
    void add_dep(str_id_t name_id) { _deps.insert(name_id); }

    void add_import(str_id_t name_id, Ref<Module> module, Ref<Class> type);
    void
    add_import(str_id_t name_id, Ref<Module> module, Ref<ClassTpl> type_tpl);

    bool resolve(sema::Resolver& resolver);

private:
    Ref<Program> _program;
    module_id_t _id;
    Ref<ast::ModuleDef> _node;
    Ptr<PersScope> _env_scope;
    Ptr<PersScope> _scope;
    SymbolTable _symbols;
    std::set<str_id_t> _deps;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam

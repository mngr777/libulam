#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module/scope.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/types.hpp>
#include <list>
#include <set>
#include <string_view>

namespace ulam::ast {
class Node;
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
class Module;

class Module {
public:
    using SymbolTable = _SymbolTable<Class, ClassTpl>;
    using Symbol = SymbolTable::Symbol;

    using ClassList = std::list<Ref<Class>>;
    using ClassTplList = std::list<Ref<ClassTpl>>;

    Module(Ref<Program> program, Ref<ast::ModuleDef> node);
    ~Module();

    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    bool operator==(const Module& other) const {
        return name_id() == other.name_id();
    }
    bool operator!=(const Module& other) const { return !operator==(other); }

    Ref<Program> program() { return _program; }

    str_id_t name_id() const;
    const std::string_view name() const;

    version_t ulam_version() const;

    Ref<ast::ModuleDef> node() { return _node; }
    Ref<const ast::ModuleDef> node() const { return _node; }

    Ref<AliasType> add_type_def(Ref<ast::TypeDef> node);
    void add_const_list(Ref<ast::VarDefList> node);
    Ref<Var> add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDef> node);
    Ref<Class> add_class(Ref<ast::ClassDef> node);
    Ref<ClassTpl> add_class_tpl(Ref<ast::ClassDef> node);

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    void export_symbols(Scope* scope);

    // TODO: add and use add_import instead
    Scope* env_scope() { return ref(_env_scope); }
    ModuleScope* scope() { return ref(_scope); }

    Symbol* get(const std::string_view name);
    const Symbol* get(const std::string_view name) const;

    Symbol* get(str_id_t name_id);
    const Symbol* get(str_id_t name_id) const;

    const ClassList classes() const { return _classes; }
    const ClassTplList class_tpls() const { return _class_tpls; }

    // TODO: move out
    bool resolve(sema::Resolver& resolver);

private:
    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _symbols.set(name_id, value);
    }

    void add_export(Ref<ast::Node> node, str_id_t name_id, Symbol* sym);

    Ref<Program> _program;
    Ref<ast::ModuleDef> _node;
    Ptr<BasicScope> _env_scope;
    Ptr<ModuleScope> _scope;
    SymbolTable _symbols;
    std::set<str_id_t> _deps;
    ClassList _classes;
    ClassTplList _class_tpls;
};

} // namespace ulam

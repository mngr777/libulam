#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <unordered_map>
#include <utility>

namespace ulam::ast {
class ArgList;
class ClassDef;
} // namespace ulam::ast

namespace ulam {

class Scope;
class Module;
class Var;
class ClassTpl;

class Class : public BasicType {
public:
    using SymbolTable = _SymbolTable<Type, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    Class(Ref<ClassTpl> tpl);
    Class(Ref<Module> module, ast::Ref<ast::ClassDef> node);
    ~Class();

    Class(Class&&) = default;
    Class& operator=(Class&&) = default;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

    str_id_t name_id() const;

    Ref<Scope> scope() { return ref(_scope); }

    void export_symbols(Scope* scope);

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

    auto node() { return _node; }

private:
    Ref<ast::ClassDef> _node;
    Ref<ClassTpl> _tpl;
    Ptr<Scope> _scope;
    SymbolTable _members;
};

class ClassTpl : public TypeTpl {
    friend Class;
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    ClassTpl(Ref<Module> module, Ref<ast::ClassDef> node);
    ~ClassTpl();

    ClassTpl(ClassTpl&&) = default;
    ClassTpl& operator=(ClassTpl&&) = default;

    Ref<Scope> def_scope() { return _def_scope; }
    Ref<Scope> scope() { return ref(_scope); }

    Ref<Type>
    type(ast::Ref<ast::ArgList> args_node, TypedValueList&& args) override;

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

    auto node() { return _node; }

private:
    Ptr<Class> inst(ast::Ref<ast::ArgList> args_node, TypedValueList&& args);

    // TMP
    std::string type_args_str(const TypedValueList& args);

    Ref<Module> _module;
    Ref<ast::ClassDef> _node;
    Ref<Scope> _def_scope;
    Ptr<Scope> _scope;
    SymbolTable _members;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

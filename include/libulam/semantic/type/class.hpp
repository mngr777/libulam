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

class Program;
class Var;

class Class : public BasicType {
public:
    using SymbolTable = _SymbolTable<Type, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    explicit Class(type_id_t id, Ref<ast::ClassDef> node);
    ~Class();

    Class(Class&&) = default;
    Class& operator=(Class&&) = default;

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }

    Symbol* set(str_id_t name_id, Ptr<Type>&& type) {
        return _members.set(name_id, std::move(type));
    }

    Symbol* set(str_id_t name_id, Ptr<Var>&& var) {
        return _members.set(name_id, std::move(var));
    }

    auto node() { return _node; }

private:
    Ref<ast::ClassDef> _node;
    SymbolTable _members;
};

class ClassTpl : public TypeTpl {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    ClassTpl(Ref<Program> program, Ref<ast::ClassDef> node): TypeTpl{program} {}

    ClassTpl(ClassTpl&&) = default;
    ClassTpl& operator=(ClassTpl&&) = default;

    Ref<Type> type(ast::Ref<ast::ArgList> arg_list, ValueList& args) override;

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }

    Symbol* set(str_id_t name_id, Ptr<Type>&& type) {
        return _members.set(name_id, std::move(type));
    }

    Symbol* set(str_id_t name_id, Ptr<TypeTpl>&& type_tpl) {
        return _members.set(name_id, std::move(type_tpl));
    }

    Symbol* set(str_id_t name_id, Ptr<Var>&& var) {
        return _members.set(name_id, std::move(var));
    }

    auto node() { return _node; }

private:
    Ref<ast::ClassDef> _node;
    SymbolTable _members;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

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
class Program;
class Var;
class ClassTpl;

class Class : public BasicType {
public:
    using SymbolTable = _SymbolTable<Type, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    Class(Ref<Program> program, Ref<ClassTpl> tpl);
    Class(Ref<Program> program, ast::Ref<ast::ClassDef> node);
    ~Class();

    Class(Class&&) = default;
    Class& operator=(Class&&) = default;

    std::string name() const override;

    bool is_class() override { return true; }
    Ref<Type> type_member(str_id_t name_id) override;

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
    Ref<Program> _program;
    Ref<ast::ClassDef> _node;
    Ref<ClassTpl> _tpl;
    SymbolTable _members;
};

class ClassTpl : public TypeTpl {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    ClassTpl(Ref<Program> program, Ref<ast::ClassDef> node): TypeTpl{program} {}

    ClassTpl(ClassTpl&&) = default;
    ClassTpl& operator=(ClassTpl&&) = default;

    std::string name() const;

    void export_symbols(Scope* scope);

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

    Ref<ast::ClassDef> _node;
    SymbolTable _members;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

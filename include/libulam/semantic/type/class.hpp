#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <unordered_map>

namespace ulam::ast {
class ArgList;
class ClassDef;
} // namespace ulam::ast

namespace ulam {

class Class : public BasicType {
public:
    using SymbolTable = _SymbolTable<Type, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    explicit Class(type_id_t id, Ref<ast::ClassDef> node);
    ~Class();

    Class(Class&&) = default;
    Class& operator=(Class&&) = default;

    Ref<ast::ClassDef> node() { return _node; }

private:
    Ref<ast::ClassDef> _node;
    SymbolTable _members;
};

class ClassTpl : public TypeTpl {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, Fun, FunTpl, Var>;
    using Symbol = SymbolTable::Symbol;

    ClassTpl(type_id_t id): TypeTpl{id} {}

    Ref<Type> type(
        Diag& diag,
        TypeIdGen& type_id_gen,
        ast::Ref<ast::ArgList> args) override;

private:
    SymbolTable _members;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

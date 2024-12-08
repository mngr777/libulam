#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
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

class Diag;
class PersScope;
class Var;

class ClassTpl;

class ClassBase {
public:
    using SymbolTable = _SymbolTable<UserType, Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    ClassBase(ast::Ref<ast::ClassDef> node, Ptr<PersScope>&& scope);

    ClassBase(ClassBase&&) = default;
    ClassBase& operator=(ClassBase&&) = default;

public:
    Ref<ClassBase> as_class_base() { return this; }

    Ref<PersScope> scope() { return ref(_scope); }

    bool has(str_id_t name_id) const { return _members.has(name_id); }

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }
    const Symbol* get(str_id_t name_id) const { return _members.get(name_id); }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

    auto node() { return _node; }
    const auto node() const { return _node; }

private:
    Ref<ast::ClassDef> _node;
    Ptr<PersScope> _scope;
    SymbolTable _members;
};

class Class : public UserType, public ClassBase {
public:
    Class(TypeIdGen& id_gen, Ref<ClassTpl> tpl);
    Class(TypeIdGen& id_gen, ast::Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~Class();

    str_id_t name_id() const override;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

private:
    Ref<ClassTpl> _tpl;
};

class ClassTpl : public TypeTpl, public ClassBase, public ScopeObject {
    friend Class;

public:
    ClassTpl(TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~ClassTpl();

    str_id_t name_id() const;

    Ref<Type>
    type(Diag& diag, ast::Ref<ast::ArgList> args_node, TypedValueList&& args) override;

private:
    Ptr<Class> inst(ast::Ref<ast::ArgList> args_node, TypedValueList&& args);

    // TMP
    std::string type_args_str(const TypedValueList& args);

    Ref<ast::ClassDef> _node;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

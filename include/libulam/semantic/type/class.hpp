#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/flags.hpp>
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
class TypeName;
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

    ClassBase(
        Ref<ast::ClassDef> node,
        Ref<Scope> module_scope,
        ScopeFlags scope_flags);

    ClassBase(ClassBase&&) = default;
    ClassBase& operator=(ClassBase&&) = default;

public:
    Ref<ClassBase> as_class_base() { return this; }

    bool has(str_id_t name_id) const { return _members.has(name_id); }

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }
    const Symbol* get(str_id_t name_id) const { return _members.get(name_id); }

    auto& members() { return _members; }
    const auto& members() const { return _members; }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

    auto node() { return _node; }
    const auto node() const { return _node; }

    Ref<PersScope> param_scope() { return ref(_param_scope); }
    // TODO: templates don't need inheritance scope
    Ref<PersScope> inh_scope() { return ref(_inh_scope); }
    Ref<PersScope> scope() { return ref(_scope); }

private:
    Ref<ast::ClassDef> _node;
    Ptr<PersScope> _param_scope;
    Ptr<PersScope> _inh_scope;
    Ptr<PersScope> _scope;
    SymbolTable _members;
};

class Class : public UserType, public ClassBase {
public:
    class Ancestor {
    public:
        Ancestor(Ref<Class> cls, Ref<ast::TypeName> node):
            _cls{cls}, _node{node} {}

        Ref<Class> cls() { return _cls; }
        Ref<const Class> cls() const { return _cls; }

        Ref<ast::TypeName> node() { return _node; }
        Ref<const ast::TypeName> node() const { return _node; }

    private:
        Ref<Class> _cls;
        Ref<ast::TypeName> _node;
    };

    Class(TypeIdGen& id_gen, Ref<ClassTpl> tpl);
    Class(TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~Class();

    str_id_t name_id() const override;

    // TMP
    auto& ancestors() { return _ancestors; }
    const auto& ancestors() const { return _ancestors; }

    void add_ancestor(Ref<Class> anc, Ref<ast::TypeName> node) {
        _ancestors.emplace_back(anc, node);
    }

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

private:
    Ref<ClassTpl> _tpl;
    std::list<Ancestor> _ancestors;
};

class ClassTpl : public TypeTpl, public ClassBase, public ScopeObject {
    friend Class;

public:
    ClassTpl(TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~ClassTpl();

    str_id_t name_id() const;

    Ref<Type> type(
        Diag& diag,
        Ref<ast::ArgList> args_node,
        TypedValueList&& args) override;

private:
    Ptr<Class> inst(Ref<ast::ArgList> args_node, TypedValueList&& args);

    // TMP
    std::string type_args_str(const TypedValueList& args);

    Ref<ast::ClassDef> _node;
    std::unordered_map<std::string, Ptr<Class>> _types;
};

} // namespace ulam

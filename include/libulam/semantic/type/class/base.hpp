#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::ast {
class ClassDef;
class FunDef;
class Param;
class TypeDef;
}

namespace ulam {

class Module;
class PersScope;

class ClassBase {
public:
    using SymbolTable = _SymbolTable<UserType, FunSet, Var, Prop>;
    using Symbol = SymbolTable::Symbol;

    ClassBase(
        Ref<ast::ClassDef> node,
        Ref<Module> module,
        ScopeFlags scope_flags);

    ClassBase(ClassBase&&) = default;
    ClassBase& operator=(ClassBase&&) = default;

public:
    ClassKind kind() const;

    bool has(str_id_t name_id) const { return _members.has(name_id); }

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }
    const Symbol* get(str_id_t name_id) const { return _members.get(name_id); }

    virtual void add_param(Ref<ast::Param> node);
    virtual void add_type_def(Ref<ast::TypeDef> node);
    virtual void add_fun(Ref<ast::FunDef> node);

    // TODO: remove
    SymbolTable& members() { return _members; }
    const SymbolTable& members() const { return _members; }

    Ref<ast::ClassDef> node() { return _node; }
    Ref<const ast::ClassDef> node() const { return _node; }

    Ref<Module> module() { return _module; }
    Ref<const Module> module() const { return _module; }

    Ref<PersScope> param_scope() { return ref(_param_scope); }
    // TODO: templates don't need inheritance scope
    Ref<PersScope> inh_scope() { return ref(_inh_scope); }
    Ref<PersScope> scope() { return ref(_scope); }

    // TODO: make protected
    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

private:
    Ref<ast::ClassDef> _node;
    Ref<Module> _module;
    Ptr<PersScope> _param_scope;
    Ptr<PersScope> _inh_scope;
    Ptr<PersScope> _scope;
    SymbolTable _members;
};

} // namespace ulam

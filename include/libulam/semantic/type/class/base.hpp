#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::ast {
class ClassDef;
}

namespace ulam {

class PersScope;

class ClassBase {
public:
    using SymbolTable = _SymbolTable<UserType, FunSet, Var, Prop>;
    using Symbol = SymbolTable::Symbol;

    ClassBase(
        Ref<ast::ClassDef> node,
        Ref<Scope> module_scope,
        ScopeFlags scope_flags);

    ClassBase(ClassBase&&) = default;
    ClassBase& operator=(ClassBase&&) = default;

public:
    bool has(str_id_t name_id) const { return _members.has(name_id); }

    Symbol* get(str_id_t name_id) { return _members.get(name_id); }
    const Symbol* get(str_id_t name_id) const { return _members.get(name_id); }

    SymbolTable& members() { return _members; }
    const SymbolTable& members() const { return _members; }

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

} // namespace ulam

#pragma once
#include <cassert>
#include <forward_list>
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <utility>
#include <vector>

namespace ulam {

class BasicScopeIter;
class Class;
class ClassTpl;
class Module;
class PersScopeView;
class ScopeIter;

// Scope base

class Scope {
protected:
    using SymbolTable = _SymbolTable<UserType, ClassTpl, FunSet, Var, Prop>;

public:
    using Symbol = SymbolTable::Symbol;
    using ItemCb = std::function<void(str_id_t, Symbol&)>;

    Scope();
    virtual ~Scope();

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    virtual Scope* parent(scope_flags_t flags = scp::NoFlags) = 0;
    const Scope* parent(scope_flags_t flags = scp::NoFlags) const;

    virtual Ref<Module> module() const;
    virtual Ref<Class> self_cls() const;
    virtual Ref<Class> eff_cls() const;
    virtual Ref<Fun> fun() const;

    virtual bool has_self() const;
    virtual LValue self() const;

    virtual scope_flags_t flags() const = 0;

    bool is(scope_flags_t flags_) const;
    bool in(scope_flags_t flags_) const;

    virtual bool has(str_id_t name_id, bool current = false) const;
    virtual Symbol* get(str_id_t name_id, bool current = false) = 0;
    const Symbol* get(str_id_t name_id, bool current = false) const;

    virtual Symbol* get_local(str_id_t name_id) = 0;

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        auto ref = ulam::ref(value);
        _decls.push_front(std::move(value));
        return do_set(name_id, Symbol{ref});
    }
    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return do_set(name_id, Symbol{value});
    }

    virtual ScopeIter begin() = 0;
    virtual ScopeIter end() = 0;

protected:
    virtual Symbol* do_set(str_id_t name_id, Symbol&& symbol) = 0;

    std::forward_list<Ptr<Decl>> _decls;
};

// ScopeBase

class ScopeBase : public Scope {
public:
    explicit ScopeBase(Scope* parent, scope_flags_t flags = scp::NoFlags):
        _parent{parent}, _flags{flags} {}

    Scope* parent(scope_flags_t flags = scp::NoFlags) override;

    scope_flags_t flags() const override { return _flags; }

    Symbol* get(str_id_t name_id, bool current = false) override;

    Symbol* get_local(str_id_t name_id) override;

protected:
    virtual Symbol* do_get_current(str_id_t name_id);
    virtual Symbol* do_get(str_id_t name_id, Ref<Class> eff_cls, bool local);

    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

    SymbolTable _symbols;

private:
    Scope* _parent;
    scope_flags_t _flags;
};

// Transient

class BasicScope : public ScopeBase {
    friend BasicScopeIter;

public:
    explicit BasicScope(Scope* parent, scope_flags_t flags = scp::NoFlags);

    BasicScope(BasicScope&&) = default;
    BasicScope& operator=(BasicScope&&) = default;

    ScopeIter begin() override;
    ScopeIter end() override;

protected:
    Symbol* do_get(str_id_t name_id, Ref<Class> eff_cls, bool local) override;
};

// Persistent

/*
 Motivation:
 Inter-class dependencies cannot be simply resolved in a single pass, consider
 ```
 quark A {
   typedef B.U R;
   typedef Int S;
 }
 quark B {
   typedef A.S T;
   typedef Int U;
 }
 ```
 Resolving A.R must be posponed until B.U is resolved. Additionally,
 we cannot incrementally resolve class members by alterating between
 classes without skipping members.

 A scope version is associated with each scope object (`Decl`),
 so a scope state at the point of definition can be later restored from any
 point, which allows to resolve dependencies recursively.
*/

class PersScopeIter;
class PersScopeView;

class PersScope : public ScopeBase {
    friend PersScopeView;

public:
    using version_t = scope_version_t;
    static constexpr version_t NoVersion = NoScopeVersion;

public:
    explicit PersScope(Scope* parent, scope_flags_t flags = scp::NoFlags):
        ScopeBase{parent, (scope_flags_t)(flags | scp::Persistent)} {}

    PersScope(PersScope&&) = default;
    PersScope& operator=(PersScope&&) = default;

    PersScopeView view(version_t version);
    PersScopeView view();

    ScopeIter begin() override;
    ScopeIter end() override;

    bool has(str_id_t name_id, bool current = false) const override;
    bool has(str_id_t name_id, version_t version, bool current = false) const;

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get_local(str_id_t name_id) override;

    str_id_t last_change(version_t version) const;

    Symbol* get(str_id_t name_id, version_t version, bool current = false);
    const Symbol*
    get(str_id_t name_id, version_t version, bool current = false) const;

    Symbol* get_local(str_id_t name_id, version_t version);
    const Symbol* get_local(str_id_t name_id, version_t version) const;

    version_t version() const;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    std::vector<str_id_t> _changes;
};

} // namespace ulam

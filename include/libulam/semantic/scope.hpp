#pragma once
#include <libulam/semantic/decl.hpp>
#include <cassert>
#include <forward_list>
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/context.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <utility>
#include <vector>

namespace ulam {

class BasicScopeIterator;
class Module;
class PersScopeView;
class ScopeIterator;

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

    virtual ScopeContextProxy ctx() = 0;
    const ScopeContextProxy ctx() const;

    virtual ScopeIterator begin() = 0;
    virtual ScopeIterator end() = 0;

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
    friend BasicScopeIterator;
public:
    explicit BasicScope(Scope* parent, scope_flags_t flags = scp::NoFlags);

    BasicScope(BasicScope&&) = default;
    BasicScope& operator=(BasicScope&&) = default;

    Ref<Class> self_cls();
    void set_self_cls(Ref<Class> cls);

    ScopeContextProxy ctx() override;

    ScopeIterator begin() override;
    ScopeIterator end() override;

protected:
    Symbol* do_get(str_id_t name_id, Ref<Class> eff_cls, bool local) override;

private:
    BasicScopeContext _ctx;
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

class PersScopeIterator;
class PersScopeView;

class PersScope : public ScopeBase {
    friend PersScopeView;

public:
    using Version = ScopeVersion;
    static constexpr Version NoVersion = NoScopeVersion;

public:
    explicit PersScope(Scope* parent, scope_flags_t flags = scp::NoFlags):
        ScopeBase{parent, (scope_flags_t)(flags | scp::Persistent)} {}

    PersScope(PersScope&&) = default;
    PersScope& operator=(PersScope&&) = default;

    PersScopeView view(ScopeVersion version);
    PersScopeView view();

    ScopeIterator begin() override;
    ScopeIterator end() override;

    bool has(str_id_t name_id, bool current = false) const override;
    bool has(str_id_t name_id, Version version, bool current = false) const;

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get_local(str_id_t name_id) override;

    ScopeContextProxy ctx() override;

    str_id_t last_change(Version version) const;

    Symbol* get(str_id_t name_id, Version version, bool current = false);
    const Symbol*
    get(str_id_t name_id, Version version, bool current = false) const;

    Symbol* get_local(str_id_t name_id, Version version);
    const Symbol* get_local(str_id_t name_id, Version version) const;

    ScopeVersion version() const;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    PersScopeContext _ctx;
    std::vector<str_id_t> _changes;
};

} // namespace ulam

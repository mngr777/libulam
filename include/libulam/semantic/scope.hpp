#pragma once
#include <cassert>
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/state.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <utility>

namespace ulam {

class Module;

// Scope base

// TODO: refactoring, proxies do not need parent/module/flags, also make flags()
// virtual instead of setting to proxy

class Scope {
protected:
    using SymbolTable = _SymbolTable<UserType, ulam::ClassTpl, ulam::Fun, Var>;

public:
    using Symbol = SymbolTable::Symbol;
    using ItemCb = std::function<void(str_id_t, Symbol&)>;

    explicit Scope(Ref<Scope> parent, ScopeFlags flags = scp::NoFlags):
        _parent{parent}, _flags{flags} {}

    Scope(): Scope{{}} {}

    virtual ~Scope(){};

    // TODO: const version
    virtual void for_each(ItemCb cb) = 0;

    ScopeFlags flags() const { return _flags; }

    bool is(ScopeFlags flags) { return _flags & flags; }

    bool in(ScopeFlags flags) {
        return is(flags) || (_parent && _parent->in(flags));
    }

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current);
    }

    virtual Symbol* get(str_id_t name_id, bool current = false) = 0;

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return do_set(name_id, Symbol{std::move(value)});
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return do_set(name_id, Symbol{value});
    }

    Ref<Scope> parent() { return _parent; }
    Ref<const Scope> parent() const { return _parent; }

    Ref<class Module> module() {
        assert(_module);
        return _module;
    }
    Ref<const class Module> module() const {
        assert(_module);
        return _module;
    }
    void set_module(Ref<class Module> module) { _module = module; }

protected:
    virtual Symbol* do_set(str_id_t name_id, Symbol&& symbol) = 0;

private:
    Ref<Scope> _parent;
    ScopeFlags _flags;
    Ref<class Module> _module;
};

// Transient

class BasicScope : public Scope {
public:
    explicit BasicScope(Ref<Scope> parent, ScopeFlags flags = scp::NoFlags):
        Scope{parent, flags} {
        assert((flags & scp::Persistent) == 0);
    }

    BasicScope(BasicScope&&) = default;
    BasicScope& operator=(BasicScope&&) = default;

    void for_each(ItemCb cb) override;

    Symbol* get(str_id_t name_id, bool current = false) override;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override {
        return _symbols.set(name_id, std::move(symbol));
    }

private:
    SymbolTable _symbols;
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

Here a scope version is associated with each  scope object (`ScopeObject`),
so a scope state at the point of definition can  be later restored from any
point, which allows to resolve dependencies recursively.
*/

class PersScope;

class PersScopeProxy : public Scope {
public:
    PersScopeProxy(Ref<PersScope> scope, ScopeVersion version);
    PersScopeProxy(PersScopeState state):
        PersScopeProxy{state.scope(), state.version()} {}
    PersScopeProxy(): Scope{}, _scope{}, _version{NoScopeVersion} {}

    void reset() { set_version(0); }
    void sync();
    std::pair<str_id_t, Symbol*> advance();

    operator bool() const { return _scope; }

    void for_each(ItemCb cb) override;

    Symbol* get(str_id_t name_id, bool current = false) override;

    str_id_t last_change() const;

    Ref<PersScope> scope() { return _scope; }

    ScopeVersion version() const { return _version; }

    void set_version(ScopeVersion version);
    void set_version_after(ScopeVersion version);

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    Ref<PersScope> _scope;
    ScopeVersion _version;
};

class PersScope : public Scope {
    friend PersScopeProxy;

public:
    using Version = std::uint32_t;
    static constexpr Version NoVersion = -1;

private:
    struct SymbolVersion {
        SymbolVersion(Version version, Symbol&& symbol):
            version{version}, symbol{std::move(symbol)} {}
        std::uint32_t version;
        Symbol symbol;
    };
    using SymbolVersionList = std::list<SymbolVersion>;
    using Map = std::unordered_map<str_id_t, SymbolVersionList>;

public:
    explicit PersScope(Ref<Scope> parent, ScopeFlags flags = scp::NoFlags):
        Scope{parent, (ScopeFlags)(flags | scp::Persistent)}, _version{0} {}

    PersScope(PersScope&&) = default;
    PersScope& operator=(PersScope&&) = default;

    PersScopeProxy proxy(ScopeVersion version) { return {this, version}; }
    PersScopeProxy proxy() { return {this, version()}; }

    void for_each(ItemCb cb) override { for_each(cb, _version); }
    void for_each(ItemCb cb, ScopeVersion version);

    bool has(str_id_t name_id, Version version, bool current = false) {
        return get(name_id, version, current);
    }

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get(str_id_t name_id, Version version, bool current = false);

    str_id_t last_change(Version version) const;
    str_id_t last_change() const { return last_change(_version); }

    ScopeVersion version() const { return _version; }

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    Version _version;
    std::unordered_map<str_id_t, SymbolVersionList> _symbols;
    std::vector<str_id_t> _changes;
};

} // namespace ulam

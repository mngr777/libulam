#pragma once
#include <cassert>
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/flags.hpp>
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

class Scope {
protected:
    using SymbolTable = _SymbolTable<UserType, ulam::ClassTpl, ulam::Fun, Var>;

public:
    using Symbol = SymbolTable::Symbol;
    using ItemCb = std::function<void(str_id_t, Symbol&)>;

    Scope() {}
    virtual ~Scope() {};

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    // TODO: const version
    virtual void for_each(ItemCb cb) = 0;

    virtual Ref<Scope> parent() = 0;
    virtual Ref<const Scope> parent() const = 0;

    bool has_version() const { return version() != NoScopeVersion; }
    virtual ScopeVersion version() const { return NoScopeVersion; }
    virtual void set_version(ScopeVersion version) { assert(false); }
    virtual void set_version_after(ScopeVersion version) { assert(false); }

    virtual ScopeFlags flags() const = 0;

    bool is(ScopeFlags flags_) { return (flags() & flags_) == flags_; }

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

protected:
    virtual Symbol* do_set(str_id_t name_id, Symbol&& symbol) = 0;
};

class ScopeBase : public Scope {
public:
    ScopeBase(Ref<Scope> parent, ScopeFlags flags = scp::NoFlags):
        Scope{}, _parent{parent}, _flags{flags} {}

    ScopeFlags flags() const override { return _flags; }

    Ref<Scope> parent() override { return _parent; }
    Ref<const Scope> parent() const override { return _parent; }

private:
    Ref<Scope> _parent;
    ScopeFlags _flags;
};

// Transient

class BasicScope : public ScopeBase {
public:
    explicit BasicScope(Ref<Scope> parent, ScopeFlags flags = scp::NoFlags):
        ScopeBase{parent, flags} {
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

class PersScopeView;

class PersScope : public ScopeBase {
    friend PersScopeView;

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
        ScopeBase{parent, (ScopeFlags)(flags | scp::Persistent)}, _version{0} {}

    PersScope(PersScope&&) = default;
    PersScope& operator=(PersScope&&) = default;

    Ptr<PersScopeView> view(ScopeVersion version);
    Ptr<PersScopeView> view();

    void for_each(ItemCb cb) override { for_each(cb, _version); }
    void for_each(ItemCb cb, ScopeVersion version);

    bool has(str_id_t name_id, Version version, bool current = false) {
        return get(name_id, version, current);
    }

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get(str_id_t name_id, Version version, bool current = false);

    str_id_t last_change(Version version) const;
    str_id_t last_change() const { return last_change(_version); }

    ScopeVersion version() const override { return _version; }

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    Version _version;
    std::unordered_map<str_id_t, SymbolVersionList> _symbols;
    std::vector<str_id_t> _changes;
};

} // namespace ulam

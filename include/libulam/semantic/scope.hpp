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
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <utility>

namespace ulam {

class Module;
class PersScopeView;

// Scope base

class Scope {
protected:
    using SymbolTable = _SymbolTable<UserType, ClassTpl, FunSet, Var, Prop>;

public:
    using Symbol = SymbolTable::Symbol;
    using ItemCb = std::function<void(str_id_t, Symbol&)>;

    Scope() {}
    virtual ~Scope(){};

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    virtual Ref<Scope> parent() = 0;
    virtual Ref<const Scope> parent() const = 0;

    bool has_version() const { return version() != NoScopeVersion; }
    virtual ScopeVersion version() const { return NoScopeVersion; }
    virtual void set_version(ScopeVersion version) { assert(false); }
    virtual void set_version_after(ScopeVersion version) { assert(false); }

    virtual Ptr<PersScopeView> view(ScopeVersion version) { assert(false); }
    virtual Ptr<PersScopeView> view() { assert(false); }

    virtual ScopeFlags flags() const = 0;

    virtual LValue self() { assert(false); }
    virtual void set_self(LValue self) { assert(false); }

    bool is(ScopeFlags flags_) { return (flags() & flags_) == flags_; }
    bool in(ScopeFlags flags_) {
        return is(flags_) || (parent() && parent()->is(flags_));
    }

    virtual bool has(str_id_t name_id, bool current = false) {
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
        Scope{}, _parent{parent}, _flags{flags}, _self{} {}

    Ref<Scope> parent() override { return _parent; }
    Ref<const Scope> parent() const override { return _parent; }

    ScopeFlags flags() const override { return _flags; }

    LValue self() override;
    void set_self(LValue self) override;

private:
    Ref<Scope> _parent;
    ScopeFlags _flags;
    LValue _self;
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

 A scope version is associated with each scope object (`Decl`),
 so a scope state at the point of definition can be later restored from any
 point, which allows to resolve dependencies recursively.
*/

class PersScopeIterator;
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

    Ptr<PersScopeView> view(ScopeVersion version) override;
    Ptr<PersScopeView> view() override;

    PersScopeIterator begin();
    PersScopeIterator end();

    bool has(str_id_t name_id, bool current = false) override {
        return has(name_id, version(), current);
    }

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

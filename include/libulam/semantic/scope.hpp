#pragma once
#include <cassert>
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

    virtual Scope* parent() = 0;
    const Scope* parent() const;

    virtual scope_flags_t flags() const = 0;

    bool is(scope_flags_t flags_) const;
    bool in(scope_flags_t flags_) const;

    virtual bool has(str_id_t name_id, bool current = false) const;
    virtual Symbol* get(str_id_t name_id, bool current = false) = 0;
    const Symbol* get(str_id_t name_id, bool current = false) const;

    Symbol* get_local(str_id_t name_id);

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return do_set(name_id, Symbol{std::move(value)});
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return do_set(name_id, Symbol{value});
    }

    virtual ScopeContextProxy ctx() = 0;
    const ScopeContextProxy ctx() const;

protected:
    virtual Symbol* do_set(str_id_t name_id, Symbol&& symbol) = 0;
};

// Transient

class BasicScope : public Scope {
public:
    explicit BasicScope(Scope* parent, scope_flags_t flags = scp::NoFlags):
        _parent{parent}, _flags{flags} {
        assert((flags & scp::Persistent) == 0);
    }

    BasicScope(BasicScope&&) = default;
    BasicScope& operator=(BasicScope&&) = default;

    Scope* parent() override { return _parent; }
    scope_flags_t flags() const override { return _flags; }

    Ref<Class> self_cls();
    void set_self_cls(Ref<Class> cls);

    Symbol* get(str_id_t name_id, bool current = false) override;

    ScopeContextProxy ctx() override;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override {
        return _symbols.set(name_id, std::move(symbol));
    }

private:
    Scope* _parent;
    scope_flags_t _flags;
    BasicScopeContext _ctx;
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

class PersScope : public Scope {
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
    explicit PersScope(
        Ref<Class> self_cls, Scope* parent, scope_flags_t flags = scp::NoFlags):
        _self_cls{self_cls},
        _parent{parent},
        _flags{(scope_flags_t)(flags | scp::Persistent)},
        _version{0} {}

    PersScope(Scope* parent, scope_flags_t flags = scp::NoFlags):
        PersScope({}, parent, flags) {}

    PersScope(PersScope&&) = default;
    PersScope& operator=(PersScope&&) = default;

    Scope* parent() override { return _parent; };
    scope_flags_t flags() const override { return _flags; }

    PersScopeView view(ScopeVersion version);
    PersScopeView view();

    PersScopeIterator begin();
    PersScopeIterator end();

    bool has(str_id_t name_id, bool current = false) const override;
    bool has(str_id_t name_id, Version version, bool current = false) const;

    Symbol* get(str_id_t name_id, bool current = false) override;

    ScopeContextProxy ctx() override;

    Symbol* get(str_id_t name_id, Version version, bool current = false);
    const Symbol*
    get(str_id_t name_id, Version version, bool current = false) const;

    str_id_t last_change(Version version) const;
    str_id_t last_change() const { return last_change(_version); }

    ScopeVersion version() const { return _version; }

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    Ref<Class> _self_cls;
    Scope* _parent;
    scope_flags_t _flags;
    Version _version;
    PersScopeContext _ctx;
    std::unordered_map<str_id_t, SymbolVersionList> _symbols;
    std::vector<str_id_t> _changes;
};

} // namespace ulam

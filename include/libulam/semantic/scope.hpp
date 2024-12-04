#pragma once
#include <cassert>
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <utility>

namespace ulam {

class Module;

// Scope base

class Scope {
protected:
    using SymbolTable = _SymbolTable<Type, TypeTpl, ulam::Fun, Var>;

public:
    using Symbol = SymbolTable::Symbol;
    using ItemCb = std::function<void(str_id_t, Symbol&)>;

    using Flag = std::uint16_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Persistent = 1;
    static constexpr Flag Program = 1 << 1;
    static constexpr Flag ModuleEnv = 1 << 2;
    static constexpr Flag Module = 1 << 3;
    static constexpr Flag Class = 1 << 4;
    static constexpr Flag ClassTpl = 1 << 5;
    static constexpr Flag Fun = 1 << 6;

    explicit Scope(Ref<Scope> parent, Flag flags = NoFlags):
        _parent{parent}, _flags{flags} {}
    virtual ~Scope() {}

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    // TODO: const version
    virtual void for_each(ItemCb cb) = 0;

    Flag flags() const { return _flags; }

    bool is(Flag flags) { return _flags & flags; }

    bool in(Flag flags) { return is(flags) || (_parent && _parent->in(flags)); }

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
    Flag _flags;
    Ref<class Module> _module;
};

// Transient

class TransScope : public Scope {
public:
    explicit TransScope(Ref<Scope> parent, Flag flags = NoFlags):
        Scope{parent, flags} {
        assert((flags & Persistent) == 0);
    }

    void for_each(ItemCb cb) override;

    Symbol* get(str_id_t name_id, bool current = false) override;

private:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override {
        return _symbols.set(name_id, std::move(symbol));
    }

    SymbolTable _symbols;
};

// Persistent

class PersScope;

class PersScopeProxy : public Scope {
public:
    explicit PersScopeProxy(Ref<PersScope> scope, ScopeVersion version);

    void reset() { set_version(0); }
    void sync();

    void for_each(ItemCb cb) override;

    Symbol* get(str_id_t name_id, bool current = false) override;

    ScopeVersion version() const { return _version; }

    void set_version(ScopeVersion version);

private:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

    Ref<PersScope> _scope;
    ScopeVersion _version;
};

class PersScope : public Scope {
    friend PersScopeProxy; // for do_set
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
    explicit PersScope(Ref<Scope> parent, Flag flags = NoFlags):
        Scope{parent, (Flag)(flags | Persistent)}, _version{0} {}

    auto proxy() { return PersScopeProxy{this, version()}; }

    void for_each(ItemCb cb) override { for_each(cb, _version); }
    void for_each(ItemCb cb, ScopeVersion version);

    bool has(str_id_t name_id, Version version, bool current = false) {
        return get(name_id, version, current);
    }

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get(str_id_t name_id, Version version, bool current = false);

    ScopeVersion version() const { return _version; }

private:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

    Version _version;
    std::unordered_map<str_id_t, SymbolVersionList> _symbols;
};

} // namespace ulam

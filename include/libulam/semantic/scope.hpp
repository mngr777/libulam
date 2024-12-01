#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <utility>

namespace ulam {

class Module;

class Scope {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, ulam::Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    using Flag = std::uint16_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Program = 1;
    static constexpr Flag Module = 1 << 1;
    static constexpr Flag Class = 1 << 2;
    static constexpr Flag ClassTpl = 1 << 3;
    static constexpr Flag Fun = 1 << 4;

    explicit Scope(Scope* parent, Flag flags = NoFlags):
        Scope({}, parent, flags) {}

    explicit Scope(
        Ref<class Module> module, Scope* parent, Flag flags = NoFlags):
        _module{module}, _parent{parent}, _flags{flags} {
        assert(!module || (flags & Module));
    }

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    Ref<class Module> module() {
        auto mod = _module ? _module : _parent->module();
        assert(mod);
        return mod;
    }

    Ref<Scope> parent() { return _parent; }

    // TODO: remove?
    Scope* copy() {
        // only safe for persistent scopes
        assert(is(Module) || is(ClassTpl) || is(Class));
        // recursively copy parent
        Ref<Scope> parent_copy{};
        if (_parent) {
            parent_copy = _parent->copy();
        }
        // make and store a copy
        auto copy = make<Scope>(parent_copy, _flags);
        auto copy_ref = ref(copy);
        _copies.push_back(std::move(copy));
        // export symbols
        copy->import_symbols(_symbols);
        return copy_ref;
    }

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    bool is(Flag flags) { return _flags & flags; }

    bool in(Flag flags) { return is(flags) || (_parent && _parent->in(flags)); }

    bool has(str_id_t name_id, Flag upto) { return get(name_id, upto); }

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current);
    }

    std::pair<Symbol*, Scope*> get_with_scope(str_id_t name_id, Flag upto) {
        auto sym = _symbols.get(name_id);
        if (sym || _flags & upto)
            return {sym, this};
        return _parent ? _parent->get_with_scope(name_id, upto)
                       : std::pair{sym, this};
    }

    std::pair<Symbol*, Scope*>
    get_with_scope(str_id_t name_id, bool in_current = false) {
        auto sym = _symbols.get(name_id);
        if (sym || in_current)
            return {sym, this};
        return _parent ? _parent->get_with_scope(name_id)
                       : std::pair{sym, this};
    }

    Symbol* get(str_id_t name_id, Flag upto) {
        auto [sym, _] = get_with_scope(name_id, upto);
        return sym;
    }

    Symbol* get(str_id_t name_id, bool in_current = false) {
        auto [sym, _] = get_with_scope(name_id, in_current);
        return sym;
    }

    // TODO: store overwritten pointers (e.g. typedef A A)

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _symbols.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _symbols.set(name_id, value);
    }

    template <typename... Ts> void import_symbols(_SymbolTable<Ts...>& st) {
        st.export_symbols(_symbols);
    }

    Flag flags() { return _flags; }

private:
    Ref<class Module> _module{};
    Scope* _parent;
    Flag _flags;
    SymbolTable _symbols;
    std::list<Ptr<Scope>> _copies;
};

} // namespace ulam

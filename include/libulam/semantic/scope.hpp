#pragma once
#include <cstdint>
#include <libulam/semantic/symbol.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class Scope {
public:
    using SymbolTable = _SymbolTable<Type, TypeTpl, ulam::Fun, Var>;
    using Symbol = SymbolTable::Symbol;

    using Flag = std::uint16_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Program = 1;
    static constexpr Flag Module = 1 << 1;
    static constexpr Flag Class = 1 << 2;
    static constexpr Flag Fun = 1 << 3;

    explicit Scope(Scope* parent, Flag flags = NoFlags):
        _parent{parent}, _flags{flags} {}

    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    bool is(Flag flags) { return _flags & flags; }

    bool in(Flag flags) { return is(flags) || (_parent && _parent->in(flags)); }

    bool has(str_id_t name_id, Flag upto) { return get(name_id, upto); }

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current);
    }

    Symbol* get(str_id_t name_id, Flag upto) {
        auto sym = _symbols.get(name_id);
        if (sym || _flags & upto)
            return sym;
        return _parent ? _parent->get(name_id, upto) : sym;
    }

    Symbol* get(str_id_t name_id, bool current = false) {
        auto sym = _symbols.get(name_id);
        if (sym || current)
            return sym;
        return _parent ? _parent->get(name_id) : sym;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _symbols.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _symbols.set(name_id, value);
    }

    template <typename... Ts>
    void
    import_symbols(_SymbolTable<Ts...>& st, bool skip_alias_types = false) {
        st.export_symbols(_symbols, skip_alias_types);
    }

    Flag flags() { return _flags; }

private:
    Scope* _parent;
    Flag _flags;
    SymbolTable _symbols; // NOTE: scope can potentially have separate symbol
                          // tables for types and funs/vars
};

} // namespace ulam

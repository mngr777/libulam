#pragma once
#include <cstdint>
#include <libulam/semantic/symbol.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class Scope {
public:
    using Flag = std::uint16_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Program = 1;
    static constexpr Flag Module = 1 << 1;
    static constexpr Flag Class = 1 << 2;

    explicit Scope(Ref<Scope> parent, Flag flags = NoFlags):
        _parent{parent}, _flags{flags} {}

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

    Flag flags() { return _flags; }

private:
    Ref<Scope> _parent;
    Flag _flags;
    SymbolTable _symbols;
};

} // namespace ulam

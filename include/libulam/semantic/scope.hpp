#pragma once
#include <cstdint>
#include <libulam/semantic/symbol.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class Scope {
public:
    using ScopeFlag = std::uint16_t;
    static const ScopeFlag NoFlags = 0;
    static const ScopeFlag Module = 1;
    static const ScopeFlag Class = 1 << 1;

    explicit Scope(Scope* parent, ScopeFlag flags = NoFlags):
        _parent{parent}, _flags{flags} {}

    bool is(ScopeFlag flags) { return _flags & flags; }

    bool has(str_id_t name_id, ScopeFlag upto) {
        return get(name_id, upto);
    }

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current);
    }

    Symbol* get(str_id_t name_id, ScopeFlag upto) {
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

    ScopeFlag flags() { return _flags; }

private:
    Scope* _parent;
    ScopeFlag _flags;
    SymbolTable _symbols;
};

} // namespace ulam

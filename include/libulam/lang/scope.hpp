#pragma once
#include <libulam/lang/symbol.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class Scope {
public:
    Scope(Ref<Scope> parent) {}

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current) != Ref<Symbol>{};
    }

    Ref<Symbol> get(str_id_t name_id, bool current = false) {
        auto sym = _symbols.get(name_id);
        if (sym || current)
            return sym;
        return _parent ? _parent->get(name_id) : sym;
    }

    template <typename T>
    Ref<Symbol> set(str_id_t name_id, Ptr<T>&& value) {
        return _symbols.set(name_id, std::move(value));
    }

private : Ref<Scope> _parent;
    SymbolTable _symbols;
};

} // namespace ulam

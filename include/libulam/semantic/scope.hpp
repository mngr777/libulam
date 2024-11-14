#pragma once
#include <libulam/semantic/symbol.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class Scope {
public:
    explicit Scope(Scope* parent) {}

    bool has(str_id_t name_id, bool current = false) {
        return get(name_id, current) != Ref<Symbol>{};
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

private:
    Scope* _parent;
    SymbolTable _symbols;
};

} // namespace ulam

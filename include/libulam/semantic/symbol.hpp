#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam {

template <typename... Ts> class _Symbol : public detail::RefPtrVariant<Ts...> {
public:
    using detail::RefPtrVariant<Ts...>::RefPtrVariant;

    _Symbol(_Symbol&&) = default;
    _Symbol& operator=(_Symbol&&) = default;

    Ref<Decl> as_decl() {
        return this->accept([](auto&& value) -> Ref<Decl> { return value; });
    }
};

template <typename... Ss> class _SymbolTable {
public:
    using Symbol = _Symbol<Ss...>;

private:
    using Map = std::unordered_map<str_id_t, Symbol>;

public:
    using iterator = typename Map::iterator;

    _SymbolTable() {}

    _SymbolTable(_SymbolTable&&) = default;
    _SymbolTable& operator=(_SymbolTable&&) = default;

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    const auto begin() const { return _symbols.begin(); }
    const auto end() const { return _symbols.end(); }

    template <typename... Ts>
    bool import_sym(str_id_t name_id, _Symbol<Ts...>& sym, bool overwrite = false) {
        return sym.accept([&](auto&& value) {
            using T = typename std::remove_pointer<
                std::decay_t<decltype(value)>>::type;
            static_assert((std::is_same_v<T, Ss> || ...));
            // import as Ref<T>
            if (overwrite || !has(name_id)) {
                set(name_id, value);
                return true;
            }
            return false;
        });
    }

    template <typename... Ts>
    void export_symbols(_SymbolTable<Ts...>& other, bool overwrite = false) {
        for (auto& pair : _symbols) {
            auto name_id = pair.first;
            auto& sym = pair.second;
            sym.accept([&](auto&& value) {
                using T = typename std::remove_pointer<
                    std::decay_t<decltype(value)>>::type;
                static_assert((std::is_same_v<T, Ts> || ...));
                // export as Ref<T>
                if (overwrite || !other.has(name_id))
                    other.set(name_id, value);
            });
        }
    }

    bool has(str_id_t name_id) const { return (_symbols.count(name_id) == 1); }

    Symbol* get(str_id_t name_id) {
        auto it = _symbols.find(name_id);
        return (it != _symbols.end()) ? &it->second : nullptr;
    }

    const Symbol* get(str_id_t name_id) const {
        auto it = _symbols.find(name_id);
        return (it != _symbols.end()) ? &it->second : nullptr;
    }

    Symbol* set(str_id_t name_id, Symbol&& sym) {
        auto [it, _] = _symbols.emplace(name_id, std::move(sym));
        return &it->second;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        auto [it, _] = _symbols.emplace(name_id, Symbol{std::move(value)});
        return &it->second;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        auto [it, _] = _symbols.emplace(name_id, Symbol{value});
        return &it->second;
    }

    void unset(str_id_t name_id) {
        assert(_symbols.count(name_id) == 1);
        _symbols.erase(name_id);
    }

private:
    Map _symbols;
};

} // namespace ulam

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace ulam {

template <typename... Ss> class _Symbol {
private:
    template <typename T> using Value = RefPtr<T>;

public:
    template <typename T> _Symbol(Ptr<T>&& value): _value{std::move(value)} {}

    template <typename T> _Symbol(Ref<T> value): _value{value} {}

    ~_Symbol() {}

    _Symbol(_Symbol&& other) = default;
    _Symbol& operator=(_Symbol&& other) = default;

    template <typename T> bool is() const {
        return std::holds_alternative<Value<T>>(_value);
    }

    template <typename T> Ref<T> get() {
        return std::get<Value<T>>(_value).ref();
    }

    Ref<ScopeObject> as_scope_obj() {
        return std::visit(
            [](auto&& value) -> Ref<ScopeObject> { return value.ref(); },
            _value);
    }

    template <typename V> void visit(V&& v) { std::visit(v, _value); } // ??

private:
    template <typename... Ts> using Variant = std::variant<Value<Ts>...>;

    Variant<Ss...> _value;
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

    template <typename... Ts> void export_symbols(_SymbolTable<Ts...>& other) {
        for (auto& pair : _symbols) {
            auto name_id = pair.first;
            auto& sym = pair.second;
            sym.visit([&](auto&& value) {
                // (static) is import possible?
                using T = typename std::decay_t<decltype(value)>::Type;
                static_assert((std::is_same_v<T, Ts> || ...));
                // export as ref
                other.set(name_id, value.ref);
            });
        }
    }

    Symbol* get(str_id_t name_id) {
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

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
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
    template <typename T> struct Value {
        using Type = T;

        Value(Ptr<T>&& val): ptr{std::move(val)}, ref{ulam::ref(ptr)} {}
        Value(Ref<T> val): ptr{}, ref{val} {}

        Value(Value&&) = default;
        Value& operator=(Value&&) = default;

        Ptr<T> ptr;
        Ref<T> ref;
    };

public:
    // TODO: remove name_id from Symbol itself?
    template <typename T>
    _Symbol(str_id_t name_id, Ptr<T>&& value):
        _name_id{name_id}, _value{std::move(value)} {}

    template <typename T>
    _Symbol(str_id_t name_id, Ref<T> value): _name_id{name_id}, _value{value} {}

    ~_Symbol() {}

    _Symbol(_Symbol&& other) = default;
    _Symbol& operator=(_Symbol&& other) = default;

    str_id_t name_id() const { return _name_id; }

    template <typename T> bool is() const {
        return std::holds_alternative<Value<T>>(_value);
    }

    template <typename T> Ref<T> get() {
        return std::get<Value<T>>(_value).ref;
    }

    template <typename V> void visit(V&& v) { std::visit(v, _value); }

private:
    template <typename... Ts> using Variant = std::variant<Value<Ts>...>;

    str_id_t _name_id;
    Variant<Ss...> _value;
};

template <typename... Ss> class _SymbolTable {
public:
    using Symbol = _Symbol<Ss...>;

    _SymbolTable() {}

    _SymbolTable(_SymbolTable&&) = default;
    _SymbolTable& operator=(_SymbolTable&&) = default;

    auto begin() { return _symbols.begin(); }
    auto end() { return _symbols.end(); }

    template <typename... Ts>
    void
    export_symbols(_SymbolTable<Ts...>& other, bool skip_alias_types = false) {
        for (auto& pair : _symbols) {
            auto name_id = pair.first;
            auto& sym = pair.second;
            // skip alias type?
            if (skip_alias_types && sym.template is<Type>() &&
                sym.template get<Type>()->basic()->is_alias())
                continue;
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

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        assert(_symbols.count(name_id) == 0);
        auto [it, _] =
            _symbols.emplace(name_id, Symbol{name_id, std::move(value)});
        return &it->second;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        assert(_symbols.count(name_id) == 0);
        auto [it, _] = _symbols.emplace(name_id, Symbol{name_id, value});
        return &it->second;
    }

    void unset(str_id_t name_id) {
        assert(_symbols.count(name_id) == 1);
        _symbols.erase(name_id);
    }

private:
    std::unordered_map<str_id_t, Symbol> _symbols;
};

} // namespace ulam

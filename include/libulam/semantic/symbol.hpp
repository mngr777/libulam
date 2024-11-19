#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <variant>

namespace ulam {

class Symbol {
private:
    struct Placeholder {};

    template <typename T> struct Value {
        Value(Ptr<T>&& val): ptr{std::move(val)}, ref{ulam::ref(ptr)} {}
        Value(Ref<T> val): ptr{}, ref{val} {}

        Ptr<T> ptr;
        Ref<T> ref;
    };

public:
    template <typename T>
    Symbol(str_id_t name_id, Ptr<T>&& value):
        _name_id{name_id}, _value{std::move(value)} {}

    template <typename T>
    Symbol(str_id_t name_id, Ref<T> value): _name_id{name_id}, _value{value} {}

    template <typename T> Symbol(str_id_t name_id, Ref<T>&& value);

    Symbol(str_id_t name_id): Symbol{name_id, Ptr<Placeholder>{}} {};
    ~Symbol();

    Symbol(Symbol&& other) = default;
    Symbol& operator=(Symbol&& other) = default;

    str_id_t name_id() const { return _name_id; }

    bool is_placeholder() const { return is<Placeholder>(); }

    template <typename T> bool is() const {
        return std::holds_alternative<Value<T>>(_value);
    }

    template <typename T> Ref<T> get() {
        return std::get<Value<T>>(_value).first;
    }

private:
    template <typename... Ts> using Variant = std::variant<Value<Ts>...>;

    str_id_t _name_id;
    Variant<Placeholder, Type, Fun, Var> _value;
};

// NOTE: scope can potentially have separate symbol tables for types and
// funs/vars
class SymbolTable {
public:
    Symbol* get(str_id_t name_id) {
        auto it = _table.find(name_id);
        return (it != _table.end()) ? &it->second : nullptr;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        assert(_table.count(name_id) == 0);
        auto [it, _] =
            _table.emplace(name_id, Symbol{name_id, std::move(value)});
        return &it->second;
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        assert(_table.count(name_id) == 0);
        auto [it, _] = _table.emplace(name_id, Symbol{name_id, value});
        return &it->second;
    }

    void set_placeholder(str_id_t name_id) {
        assert(_table.count(name_id) == 0);
        _table.emplace(name_id, Symbol{name_id});
    }

    void unset(str_id_t name_id) {
        assert(_table.count(name_id) == 1);
        _table.erase(name_id);
    }

    void unset_placeholders();

private:
    std::unordered_map<str_id_t, Symbol> _table;
};

} // namespace ulam

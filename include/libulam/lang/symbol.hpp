#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam {

class Type;
class Fun;
class Var;

class Symbol {
public:
    template <typename T>
    Symbol(str_id_t name_id, Ptr<T>&& value):
        _name_id{name_id}, _value{std::move(value)} {}
    ~Symbol();

    str_id_t name_id() const { return _name_id; }

    template <typename T> bool is() { return is<T>(_value); }

    template <typename T> Ref<T> get() { return as_ref<T>(_value); }

private:
    str_id_t _name_id;
    Variant<Type, Fun, Var> _value;
};

class SymbolTable {
public:
    Symbol* get(str_id_t name_id);

    template <typename T> void set(str_id_t name_id, Ptr<T>&& value) {
        assert(_table.count(name_id) == 0);
        _table.emplace(name_id, Symbol(name_id, std::move(value)));
    }

    void unset(str_id_t name_id);

private:
    std::unordered_map<str_id_t, Symbol> _table;
};

} // namespace ulam

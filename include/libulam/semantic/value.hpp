#pragma once
#include <cstdint>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/object.hpp>
#include <list>
#include <string>
#include <variant>

namespace ulam {

using Integer = std::int64_t;
using Unsigned = std::uint64_t;
using String = std::string;

class Type;
class Var;

template <typename... Ts> class _Value {
public:
    _Value() {}

    bool is_unknown() const { return is<std::monostate>(); }

    template <typename T> bool is() const {
        return std::holds_alternative<T>(_value);
    }

    template <typename T> T& get() { return std::get<T>(_value); }
    template <typename T> const T& get() const { return std::get<T>(_value); }

    template <typename T> void set(T&& value) { _value = value; }

private:
    std::variant<std::monostate, Ts...> _value;
};

class RValue : public _Value<Integer, Unsigned, Bits, String> {
public:
    RValue(): _Value() {}
};

class LValue : public _Value<Ref<Var>> { // TODO: array access
public:
    LValue(): _Value() {}

    RValue* rvalue() { return nullptr; } // TODO
};

class Value {
public:
    bool is_nil() const {
        return std::holds_alternative<std::monostate>(_value);
    }

    bool is_lvalue() const { return std::holds_alternative<LValue>(_value); }

    LValue* lvalue() {
        if (is_nil() || !is_lvalue())
            return nullptr;
        return &std::get<LValue>(_value);
    }
    RValue* rvalue() {
        return is_lvalue() ? lvalue()->rvalue() : &std::get<RValue>(_value);
    }

private:
    std::variant<std::monostate, LValue, RValue> _value;
};

using ValueList = std::list<Value>;

} // namespace ulam

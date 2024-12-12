#pragma once
#include <cstdint>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/object.hpp>
#include <list>
#include <string>
#include <utility>
#include <variant>

#define ULAM_MAX_INT_SIZE (sizeof(Integer) * 8)

namespace ulam {

using Integer = std::int64_t;
using Unsigned = std::uint64_t;
using Bool = bool;
using String = std::string;

class Var;

template <typename... Ts> class _Value {
public:
    template <typename T> _Value(T&& value): _value{std::forward<T>(value)} {}
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

class RValue
    : public _Value<Integer, Unsigned /* or Unary */, Bool, Bits, String> {
public:
    template <typename T> RValue(T&& value): _Value{std::forward<T>(value)} {}
    RValue(): _Value{} {}
};

class LValue : public _Value<Ref<Var>> { // TODO: array/member access
public:
    template <typename T> LValue(T&& value): _Value{std::forward<T>(value)} {}
    LValue(): _Value{} {}

    RValue* rvalue();
    const RValue* rvalue() const;
};

class Value {
public:
    Value() {}
    Value(LValue&& lvalue): _value{std::move(lvalue)} {}
    Value(RValue&& rvalue): _value{std::move(rvalue)} {}

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    operator bool() { return !is_nil(); }

    bool is_nil() const {
        return std::holds_alternative<std::monostate>(_value);
    }

    bool is_lvalue() const { return std::holds_alternative<LValue>(_value); }

    LValue* lvalue() {
        return const_cast<LValue*>(std::as_const(*this).lvalue());
    }

    const LValue* lvalue() const {
        if (is_nil() || !is_lvalue())
            return nullptr;
        return &std::get<LValue>(_value);
    }

    RValue* rvalue() {
        return const_cast<RValue*>(std::as_const(*this).rvalue());
    }

    const RValue* rvalue() const {
        return is_lvalue() ? lvalue()->rvalue() : &std::get<RValue>(_value);
    }

private:
    std::variant<std::monostate, LValue, RValue> _value;
};

using ValueList = std::list<Value>;

} // namespace ulam

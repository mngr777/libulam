#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>
#include <utility>
#include <variant>

namespace ulam {

class FunSet;
class Type;
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

    template <typename V> void accept(V&& visitor) {
        std::visit(visitor, _value);
    }

private:
    std::variant<std::monostate, Ts...> _value;
};

class RValue : public _Value<
                   Integer,
                   Unsigned /* Unary, Bool */,
                   Bits,
                   String,
                   Ref<FunSet>,
                   SPtr<Object>> {
public:
    template <typename T> RValue(T&& value): _Value{std::forward<T>(value)}
    {} RValue(): _Value{} {}
};

class LValue
    : public _Value<Ref<Var>, BoundFunSet, BoundProp /* TODO: array access */> {
public:
    template <typename T> LValue(T&& value): _Value{std::forward<T>(value)} {}
    LValue(): _Value{} {}

    Ref<Type> type();

    RValue* rvalue();
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
        assert(!is_nil());
        return is_lvalue() ? const_cast<LValue*>(lvalue())->rvalue()
                           : &std::get<RValue>(_value);
    }

private:
    std::variant<std::monostate, LValue, RValue> _value;
};

using ValueList = std::list<Value>;

} // namespace ulam

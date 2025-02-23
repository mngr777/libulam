#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <list>
#include <utility>

namespace ulam {

template <typename T> class _TypedValue {
public:
    _TypedValue(): _type{}, _value{} {}

    _TypedValue(Ref<T> type, Value&& value):
        _type{type}, _value{std::move(value)} {}

    _TypedValue(_TypedValue&&) = default;
    _TypedValue& operator=(_TypedValue&&) = default;

    operator bool() { return _type; }

    Ref<T> type() { return _type; }
    Ref<const T> type() const { return _type; }

    Value& value() { return _value; }
    const Value& value() const { return _value; }

    Value move_value() {
        Value val;
        std::swap(val, _value);
        return val;
    }

    std::pair<Ref<Type>, Value> move() { return {_type, move_value()}; }

private:
    Ref<T> _type;
    Value _value;
};

class Type;

class TypedValue : public _TypedValue<Type> {
public:
    using _TypedValue::_TypedValue;
};

using TypedValueList = std::list<TypedValue>;

} // namespace ulam

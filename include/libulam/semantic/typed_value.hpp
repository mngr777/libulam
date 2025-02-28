#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <list>
#include <utility>

namespace ulam {

class Type;

class TypedValue {
public:
    TypedValue(): _type{}, _value{} {}

    TypedValue(Ref<Type> type, Value&& value):
        _type{type}, _value{std::move(value)} {}

    TypedValue(TypedValue&&) = default;
    TypedValue& operator=(TypedValue&&) = default;

    operator bool() { return _type; }

    Ref<Type> type() const { return _type; }

    Value& value() { return _value; }
    const Value& value() const { return _value; }

    Value move_value() {
        Value val;
        std::swap(val, _value);
        return val;
    }

    std::pair<Ref<Type>, Value> move() { return {_type, move_value()}; }

private:
    Ref<Type> _type;
    Value _value;
};

using TypedValueList = std::list<TypedValue>;

} // namespace ulam

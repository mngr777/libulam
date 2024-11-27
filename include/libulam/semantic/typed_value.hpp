#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <list>
#include <utility>

namespace ulam {

class Type;

// TODO: Is it better to have type in value class itself?
class TypedValue {
public:
    TypedValue(): _type{}, _value{} {}

    TypedValue(Ref<Type> type, Value&& value):
        _type{type}, _value{std::move(value)} {}

    TypedValue(TypedValue&&) = default;
    TypedValue& operator=(TypedValue&&) = default;

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }

    const Value& value() const { return _value; }

    Value move_value() {
        Value val;
        std::swap(val, _value);
        return val;
    }

private:
    Ref<Type> _type;
    Value _value;
};

using TypedValueList = std::list<TypedValue>;

} // namespace ulam

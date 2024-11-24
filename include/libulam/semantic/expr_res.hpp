#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <utility>

namespace ulam {

enum class ExprError {
    Ok,
    NotImplemented,
    NoOperator,
    CastRequired,
    InvalidCast
};

class Type;

class ExprRes {
public:
    ExprRes(Ref<Type> type, Value&& value, ExprError error = ExprError::Ok):
        _type{type}, _value{std::move(value)}, _error{error} {}

    ExprRes(ExprError error = ExprError::NotImplemented):
        _type{Ref<Type>{}}, _error{error} {}

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }

    Value& value() { return _value; }
    const Value& value() const { return _value; }

    ExprError error() const { return _error; }

private:
    Ref<Type> _type;
    Value _value;
    ExprError _error;
};

} // namespace ulam

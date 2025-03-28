#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <utility>

namespace ulam {

enum class ExprError {
    Ok,
    Error,
    NoSelf,
    NoSuper,
    BaseNotFound,
    SymbolNotFound,
    MemberNotFound,
    UnresolvableType,
    UnresolvableClassConst,
    UnresolvableVar,
    UnresolvableProp,
    UnresolvableFunSet,
    NotAssignable,
    NotImplemented,
    NotFunction,
    NotClassConst,
    NotArray,
    TypeMismatch,
    UnknownArrayIndex,
    ArrayIndexOutOfRange,
    CharIndexOutOfRange,
    NotObject,
    NotClass,
    NoOperator,
    InvalidTypeOperator,
    InvalidOperandType,
    CastRequired,
    InvalidCast,
    NonVoidReturn,
    NoReturn,
    NoReturnValue,
    NotReference,
    ReferenceToLocal,
    InvalidReturnType,
    NoMatchingFunction,
    FunctionIsPureVirtual,
    AmbiguousFunctionCall
};

class Type;

// TODO: move to sema/eval

class ExprRes {
public:
    ExprRes(TypedValue&& tv, ExprError error = ExprError::Ok):
        _typed_value{std::move(tv)}, _error{error} {}

    ExprRes(Ref<Type> type, Value&& value, ExprError error = ExprError::Ok):
        _typed_value{type, std::move(value)}, _error{error} {}

    ExprRes(ExprError error = ExprError::NotImplemented): _error{error} {}

    ExprRes(ExprRes&&) = default;
    ExprRes& operator=(ExprRes&&) = default;

    bool ok() const { return _error == ExprError::Ok; }
    operator bool() { return ok(); }

    Ref<Type> type() { return _typed_value.type(); }
    Ref<const Type> type() const { return _typed_value.type(); }

    const Value& value() { return _typed_value.value(); }

    const TypedValue& typed_value() { return _typed_value; }

    TypedValue move_typed_value() {
        TypedValue tv;
        std::swap(tv, _typed_value);
        return tv;
    }

    Value move_value() { return _typed_value.move_value(); }

    ExprError error() const { return _error; }

private:
    TypedValue _typed_value;
    ExprError _error;
};

} // namespace ulam

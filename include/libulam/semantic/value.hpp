#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>
#include <type_traits>
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

class RValue : public detail::Variant<
                   Integer,
                   Unsigned /* Unary, Bool*/,
                   Bits,
                   String,
                   Ref<FunSet>, /* ?? */
                   SPtr<Object>> {
public:
    using Variant::Variant;

    RValue(RValue&&) = default;
    RValue& operator=(RValue&&) = default;

    RValue copy() const;
};

class LValue : public detail::Variant<
                   Ref<Var>,
                   ObjectView,
                   BoundFunSet,
                   BoundProp /* TODO: object ref, array access */> {
public:
    using Variant::Variant;

    Ref<Type> type();

    RValue rvalue() const;
};

class Value : public detail::Variant<LValue, RValue> {
public:
    using RValueCb = std::function<void(RValue&)>;
    using RValueConstCb = std::function<void(const RValue&)>;

    using Variant::Variant;

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    bool is_lvalue() const { return is<LValue>(); }
    bool is_rvalue() const { return is<RValue>(); }

    LValue* lvalue() {
        return const_cast<LValue*>(std::as_const(*this).lvalue());
    }

    const LValue* lvalue() const {
        if (empty() || !is_lvalue())
            return nullptr;
        return &const_cast<Value*>(this)->get<LValue>();
    }

    RValue copy_rvalue() const;
    RValue move_rvalue();
};

using ValueList = std::list<Value>;

} // namespace ulam

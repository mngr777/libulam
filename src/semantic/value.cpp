#include <libulam/detail/variant.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() {
    return {}; // TODO
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<Var> var) { return var->rvalue(); },
        [&](const BoundProp& bound_prop) {
            RValue rvalue{};
            bound_prop.load(rvalue);
            return rvalue;
        },
        [&](std::monostate) { return RValue{}; },
        [&](auto&&) -> RValue { assert(false); });
}

RValue RValue::copy_shallow() const {
    return accept([&](auto&& value) { return RValue{value}; });
}

// Value

RValue Value::rvalue() const {
    return std::visit(
        detail::variant::Overloads{
            [&](const LValue& lval) { return lval.rvalue(); },
            [&](const RValue& rval) { return rval.copy_shallow(); },
            [&](const std::monostate& val) { return RValue{}; }},
        _value);
}

} // namespace ulam

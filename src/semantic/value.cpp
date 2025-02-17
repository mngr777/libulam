#include "libulam/semantic/value/types.hpp"
#include <libulam/detail/variant.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() { return {}; }

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<Var> var) { return var->rvalue(); },
        [&](const BoundProp& bound_prop) {
            RValue rvalue{};
            bound_prop.load(rvalue);
            return rvalue;
        },
        [&](std::monostate) { return RValue{}; },
        [&](auto&) -> RValue { assert(false); });
}

LValue LValue::array_access(Ref<Type> item_type, array_idx_t index) {
    return LValue{accept(
        [&](Ref<Var> var) {
            return ArrayAccess{var->array_view(), item_type, index};
        },
        [&](ArrayAccess& array_access) {
            return array_access.item_array_access(index);
        },
        [&](BoundProp& bound_prop) {
            return ArrayAccess{bound_prop.mem_array_view(), item_type, index};
        },
        [&](auto& other) -> ArrayAccess { assert(false); })};
}

// RValue

RValue RValue::copy() const {
    return accept(
        [&](const Bits& bits) { return RValue{bits.copy()}; },
        [&](const Array& array) { return RValue{array.copy()}; },
        [&](SPtr<const Object> obj) { return RValue{obj->copy()}; },
        [&](auto value) { return RValue{value}; });
}

RValue RValue::array_access(Ref<Type> item_type, array_idx_t index) {
    return accept(
        [&](const Array& array) { return array.load(item_type, index); },
        [&](auto& other) -> RValue { assert(false); });
}

// Value

Value Value::array_access(Ref<Type> item_type, array_idx_t index) {
    return accept(
        [&](LValue& lval) {
            return Value{lval.array_access(item_type, index)};
        },
        [&](RValue& rval) {
            return Value{rval.array_access(item_type, index)};
        },
        [&](auto& other) -> Value { assert(false); });
}

RValue Value::copy_rvalue() const {
    return accept(
        [&](const LValue& lval) { return lval.rvalue(); },
        [&](const RValue& rval) { return rval.copy(); },
        [&](const std::monostate& val) { return RValue{}; });
}

RValue Value::move_rvalue() {
    return accept(
        [&](LValue& lval) { return lval.rvalue(); },
        [&](RValue& rval) {
            RValue res{};
            std::swap(res, rval);
            return res;
        },
        [&](const std::monostate& val) { return RValue{}; });
}

} // namespace ulam

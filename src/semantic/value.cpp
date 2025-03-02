#include "libulam/semantic/value/bound_fun_set.hpp"
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() const {
    return accept(
        [&](Ref<const Var> var) { return var->type(); },
        [&](const DataView& data) { return data.type(); },
        [&](const BoundFunSet& bound_fset) -> Ref<Type> { assert(false); },
        [&](const std::monostate&) -> Ref<Type> { assert(false); });
}

DataView LValue::data_view() {
    return accept(
        [&](Ref<Var> var) { return var->data_view(); },
        [&](DataView& data) { return data; },
        [&](BoundFunSet&) -> DataView { assert(false); },
        [&](std::monostate&) -> DataView { assert(false); });
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<const Var> var) { return var->rvalue(); },
        [&](const DataView& data) { return data.load(); },
        [&](const BoundFunSet&) -> RValue { assert(false); },
        [&](const std::monostate&) -> RValue { assert(false); });
}

LValue LValue::array_access(array_idx_t idx) {
    return derived(data_view().array_item(idx));
}

LValue LValue::prop(Ref<Prop> prop) { return derived(data_view().prop(prop)); }

LValue LValue::bound_fset(Ref<FunSet> fset) {
    return derived(BoundFunSet{data_view(), fset});
}

LValue LValue::self() {
    LValue lval{derived(accept(
        [&](Ref<Var> var) { return var->data_view(); },
        [&](DataView& data) { return data; },
        [&](BoundFunSet& bfset) { return bfset.self(); },
        [&](std::monostate&) -> DataView { assert(false); }))};
    lval.set_scope_lvl(AutoScopeLvl);
    lval.set_is_xvalue(false);
    return lval;
}

Value LValue::assign(RValue&& rval) {
    return accept(
        [&](Ref<Var> var) {
            var->set_value(Value{std::move(rval)});
            return Value{var->lvalue()};
        },
        [&](DataView& data) {
            data.store(std::move(rval));
            return Value{derived(data)};
        },
        [&](const BoundFunSet&) -> Value { assert(false); },
        [&](const std::monostate& none) -> Value {
            // pretend assign for empty value
            return Value{derived(none)};
        });
}

// RValue

RValue RValue::copy() const {
    return accept(
        [&](const Bits& bits) { return RValue{bits.copy(), is_consteval()}; },
        [&](DataPtr data) { return RValue{data->copy(), is_consteval()}; },
        [&](auto value) { return RValue{value, is_consteval()}; });
}

DataView RValue::data_view() {
    return accept(
        [&](DataPtr data) { return data->view(); },
        [&](auto& other) -> DataView { assert(false); });
}

LValue RValue::array_access(array_idx_t idx) {
    return accept(
        [&](DataPtr& data) { return LValue{data->array_item(idx)}; },
        [&](auto& other) -> LValue { assert(false); });
}

LValue RValue::prop(Ref<Prop> prop) {
    return accept(
        [&](DataPtr data) { return LValue{data->prop(prop)}; },
        [&](auto& other) -> LValue { assert(false); });
}

LValue RValue::bound_fset(Ref<FunSet> fset) {
    return accept(
        [&](DataPtr data) { return LValue{BoundFunSet{data->view(), fset}}; },
        [&](auto& other) -> LValue { assert(false); });
}

LValue RValue::self() {
    LValue lval{accept(
        [&](DataPtr& data) { return data->view(); },
        [&](auto& other) -> DataView { assert(false); })};
    lval.set_scope_lvl(AutoScopeLvl);
    lval.set_is_xvalue(false);
    return lval;
}

// Value

bool Value::empty() const {
    return accept([&](auto& val) { return val.empty(); });
}

DataView Value::data_view() {
    return accept([&](auto& val) { return val.data_view(); });
}

Value Value::array_access(array_idx_t idx) {
    return accept([&](auto& val) { return Value{val.array_access(idx)}; });
}

Value Value::prop(Ref<Prop> prop) {
    return accept([&](auto& val) { return Value{val.prop(prop)}; });
}

Value Value::bound_fset(Ref<FunSet> fset) {
    return accept([&](auto& val) { return Value{val.bound_fset(fset)}; });
}

LValue Value::self() {
    return accept([&](auto& val) { return val.self(); });
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
        });
}

bool Value::is_consteval() const {
    return accept([&](const auto& val) { return val.is_consteval(); });
}

void Value::with_rvalue(std::function<void(const RValue&)> cb) const {
    accept(
        [&](const LValue& lval) {
            // TODO: LValue::with_rvalue
            auto rval = lval.rvalue();
            cb(rval);
        },
        [&](const RValue& rval) { cb(rval); });
}

} // namespace ulam

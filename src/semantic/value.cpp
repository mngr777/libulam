#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var.hpp>
#include <variant>

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
        [&](BoundFunSet&) { return DataView{}; },
        [&](std::monostate&) { return DataView{}; });
}

const DataView LValue::data_view() const {
    return const_cast<LValue*>(this)->data_view();
}

Ref<Class> LValue::dyn_cls(bool real) const {
    auto data = data_view();
    return data.is_class() ? data.type(real)->canon()->as_class()
                           : Ref<Class>{};
}

Ref<Type> LValue::dyn_obj_type(bool real) const {
    auto data = data_view();
    assert(data.is_object());
    return data.type(real);
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

LValue LValue::as(Ref<Type> type) {
    auto data = data_view();
    if (data)
        data = data.as(type);
    return derived(data);
}

LValue LValue::atom_of() {
    auto data_view = accept(
        [&](Ref<Var> var) { return var->data_view().atom_of(); },
        [&](DataView& data) { return data.atom_of(); },
        [&](BoundFunSet& bfset) { return DataView{}; },
        [&](std::monostate&) { return DataView{}; });
    if (!data_view)
        return LValue{};
    return derived(data_view);
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<const Var> var) { return var->rvalue(); },
        [&](const DataView& data) { return data.load(); },
        [&](const BoundFunSet&) -> RValue { assert(false); },
        [&](const std::monostate&) { return RValue{}; });
}

LValue LValue::array_access(array_idx_t idx) {
    auto data = data_view();
    if (!data)
        return derived(std::monostate{});
    return derived(data.array_item(idx));
}

LValue LValue::prop(Ref<Prop> prop) {
    auto data = data_view();
    if (!data)
        return derived(std::monostate{});
    return derived(data_view().prop(prop));
}

LValue LValue::bound_fset(Ref<FunSet> fset) {
    return derived(BoundFunSet{data_view(), fset});
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

bool LValue::is_consteval() const {
    return accept(
        [&](Ref<const Var> var) { return var->is_consteval(); },
        [&](const DataView& data) {
            return false; // TODO
        },
        [&](const BoundFunSet&) {
            return true; // ??
        },
        [&](const std::monostate&) { return false; });
}

// RValue

RValue RValue::copy() const {
    if (empty())
        return RValue{};
    return accept(
        [&](const Bits& bits) { return RValue{bits.copy(), is_consteval()}; },
        [&](DataPtr data) { return RValue{data->copy(), is_consteval()}; },
        [&](auto value) { return RValue{value, is_consteval()}; });
}

DataView RValue::data_view() {
    return accept(
        [&](DataPtr data) { return data->view(); },
        [&](auto& other) { return DataView{}; });
}

const DataView RValue::data_view() const {
    return const_cast<RValue*>(this)->data_view();
}

Ref<Class> RValue::dyn_cls(bool real) const {
    auto data = data_view();
    return data.is_class() ? data.type(real)->canon()->as_class()
                           : Ref<Class>{};
}

Ref<Type> RValue::dyn_obj_type(bool real) const {
    auto data = data_view();
    assert(data.is_object());
    return data.type();
}

LValue RValue::self() {
    LValue lval{accept(
        [&](DataPtr& data) { return data->view(); },
        [&](auto& other) -> DataView { assert(false); })};
    lval.set_scope_lvl(AutoScopeLvl);
    lval.set_is_xvalue(false);
    return lval;
}

LValue RValue::as(Ref<Type> type) {
    LValue lval{accept(
        [&](DataPtr& data) { return data->view(); },
        [&](auto& other) -> DataView { assert(false); })};
    lval.set_scope_lvl(AutoScopeLvl);
    return lval;
}

LValue RValue::atom_of() {
    auto data = accept(
        [&](DataPtr& data) { return data->view(); },
        [&](auto& other) { return DataView{}; });
    if (!data)
        return LValue{};
    return LValue{data.atom_of()};
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

// Value

bool Value::empty() const {
    return accept([&](auto& val) { return val.empty(); });
}

DataView Value::data_view() {
    return accept([&](auto& val) { return val.data_view(); });
}

const DataView Value::data_view() const {
    return accept([&](const auto& val) { return val.data_view(); });
}

Ref<Class> Value::dyn_cls(bool real) const {
    return accept([&](const auto& val) { return val.dyn_cls(real); });
}

Ref<Type> Value::dyn_obj_type(bool real) const {
    return accept([&](const auto& val) { return val.dyn_obj_type(real); });
}

LValue Value::self() {
    return accept([&](auto& val) { return val.self(); });
}

LValue Value::as(Ref<Type> type) {
    return accept([&](auto& val) { return val.as(type); });
}

LValue Value::atom_of() {
    return accept([&](auto& val) { return val.atom_of(); });
}

bitsize_t Value::position_of() {
    auto data = data_view();
    return data ? data.position_of() : NoBitsize;
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

RValue Value::copy_rvalue() const {
    return accept(
        [&](const LValue& lval) { return lval.rvalue(); },
        [&](const RValue& rval) { return rval.copy(); });
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

Value Value::deref() { return Value{move_rvalue()}; }

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

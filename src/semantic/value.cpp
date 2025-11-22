#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>
#include <variant>

namespace ulam {

// LValue

LValue::LValue(): Base{std::monostate{}} {}

LValue::LValue(std::monostate): Base{std::monostate{}} {}

LValue::LValue(Ref<Var> var): Base{var}, _is_consteval{var->is_consteval()} {}

LValue::LValue(DataView data): Base{data} {}

LValue::LValue(BoundFunSet bfset): Base{bfset} {}

bool LValue::has_rvalue() const {
    return accept(
        [&](Ref<const Var> var) { return var->value().has_rvalue(); },
        [&](const DataView& data) { return true; },
        [&](const BoundFunSet&) -> bool { assert(false); },
        [&](const std::monostate&) { return false; });
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<const Var> var) { return var->rvalue(); },
        [&](const DataView& data) {
            auto rval = data.load(true /* see t3697 */);
            rval.set_is_consteval(is_consteval());
            return rval;
        },
        [&](const BoundFunSet&) -> RValue { assert(false); },
        [&](const std::monostate&) { return RValue{}; });
}

void LValue::with_rvalue(std::function<void(const RValue&)> cb) const {
    accept(
        [&](Ref<const Var> var) { var->value().with_rvalue(cb); },
        [&](const DataView& data) {
            RValue rval = data.load();
            rval.set_is_consteval(_is_consteval);
            cb(rval);
        },
        [&](const BoundFunSet& bound_fset) { assert(false); },
        [&](const std::monostate&) {
            RValue rval;
            cb(rval);
        });
}

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
    auto type = dyn_obj_type(real);
    return type->is_class() ? type->as_class() : Ref<Class>{};
}

Ref<Type> LValue::dyn_obj_type(bool real) const {
    auto type = data_view().type(real);
    assert(type->is_object());
    return type;
}

LValue LValue::self() {
    LValue lval{derived(accept(
        [&](Ref<Var> var) { return LValue{var->data_view()}; },
        [&](DataView& data) { return LValue{data}; },
        [&](BoundFunSet& bfset) {
            return bfset.has_self() ? LValue{bfset.self()} : LValue{};
        },
        [&](std::monostate&) -> LValue { assert(false); }))};
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

LValue LValue::array_access(array_idx_t idx, bool is_consteval_idx) {
    auto data = data_view();
    if (!data || idx == UnknownArrayIdx)
        return derived(std::monostate{});

    auto lval = derived(data.array_item(idx));
    lval.set_is_consteval(is_consteval() && is_consteval_idx);
    return lval;
}

const LValue
LValue::array_access(array_idx_t idx, bool is_consteval_idx) const {
    return const_cast<LValue*>(this)->array_access(idx, is_consteval_idx);
}

LValue LValue::prop(Ref<Prop> prop) {
    auto data = data_view();
    if (!data)
        return derived(std::monostate{});
    return derived(data.prop(prop));
}

const LValue LValue::prop(Ref<Prop> prop) const {
    return const_cast<LValue*>(this)->prop(prop);
}

LValue LValue::bound_fset(Ref<FunSet> fset, Ref<Class> base) {
    return derived(BoundFunSet{data_view(), fset, base});
}

Value LValue::assign(RValue&& rval) {
    return accept(
        [&](Ref<Var> var) {
            var->set_rvalue(std::move(rval));
            return Value{derived(var->lvalue())};
        },
        [&](DataView& data) {
            data.store(std::move(rval));
            LValue lval = derived(data);
            lval.set_is_consteval(false);
            return Value{lval};
        },
        [&](const BoundFunSet&) -> Value { assert(false); },
        [&](const std::monostate&) -> Value {
            // pretend assign for empty value
            return Value{derived(std::monostate{})};
        });
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
    auto type = dyn_obj_type(real);
    return type->is_class() ? type->as_class() : Ref<Class>{};
}

Ref<Type> RValue::dyn_obj_type(bool real) const {
    auto type = data_view().type(real);
    assert(type->is_object());
    return type;
}

LValue RValue::self() {
    LValue lval;
    if (!empty()) {
        lval = LValue{accept(
            [&](DataPtr& data) { return data->view(); },
            [&](auto& other) -> DataView { assert(false); })};
    }
    lval.set_scope_lvl(AutoScopeLvl);
    lval.set_is_xvalue(false);
    return lval;
}

LValue RValue::as(Ref<Type> type) {
    LValue lval{accept(
        [&](DataPtr& data) { return data->view().as(type); },
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

LValue RValue::array_access(array_idx_t idx, bool is_consteval_idx) {
    bool is_consteval_ = is_consteval() && is_consteval_idx;
    return accept(
        [&](DataPtr& data) {
            LValue lval{data->array_item(idx)};
            lval.set_is_consteval(is_consteval_);
            return lval;
        },
        [&](std::monostate) {
            assert(idx == UnknownArrayIdx);
            return LValue{};
        },
        [&](auto& other) -> LValue { assert(false); });
}

const LValue
RValue::array_access(array_idx_t idx, bool is_consteval_idx) const {
    return const_cast<RValue*>(this)->array_access(idx, is_consteval_idx);
}

LValue RValue::prop(Ref<Prop> prop) {
    return accept(
        [&](DataPtr data) {
            LValue lval{data->prop(prop)};
            lval.set_is_consteval(is_consteval());
            return lval;
        },
        [&](std::monostate) { return LValue{}; },
        [&](auto& other) -> LValue { assert(false); });
}

const LValue RValue::prop(Ref<Prop> prop) const {
    return const_cast<RValue*>(this)->prop(prop);
}

LValue RValue::bound_fset(Ref<FunSet> fset, Ref<Class> base) {
    return accept(
        [&](DataPtr data) {
            return LValue{BoundFunSet{data->view(), fset, base}};
        },
        [&](std::monostate) {
            return LValue{BoundFunSet{DataView{}, fset, base}};
        },
        [&](auto& other) -> LValue { assert(false); });
}

// Value

bool Value::empty() const {
    return accept([&](auto& val) { return val.empty(); });
}

bool Value::has_rvalue() const {
    return accept(
        [&](const LValue& lval) { return lval.has_rvalue(); },
        [&](const RValue& rval) { return !rval.empty(); });
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

Value Value::array_access(array_idx_t idx, bool is_consteval_idx) {
    return accept([&](auto& val) {
        return Value{val.array_access(idx, is_consteval_idx)};
    });
}

const Value Value::array_access(array_idx_t idx, bool is_consteval_idx) const {
    return const_cast<Value*>(this)->array_access(idx, is_consteval_idx);
}

Value Value::prop(Ref<Prop> prop) {
    return accept([&](auto& val) { return Value{val.prop(prop)}; });
}

const Value Value::prop(Ref<Prop> prop) const {
    return const_cast<Value*>(this)->prop(prop);
}

Value Value::bound_fset(Ref<FunSet> fset, Ref<Class> base) {
    return accept([&](auto& val) { return Value{val.bound_fset(fset, base)}; });
}

Value Value::copy() const {
    return is_lvalue() ? Value{lvalue()} : Value{copy_rvalue()};
}

RValue Value::copy_rvalue() const {
    return accept(
        [&](const LValue& lval) { return lval.rvalue(); },
        [&](const RValue& rval) { return rval.copy(); });
}

RValue Value::move_rvalue() {
    return accept(
        [&](LValue& lval) { return lval.rvalue(); },
        [&](RValue& rval) { return std::exchange(rval, {}); });
}

Value Value::deref() { return Value{move_rvalue()}; }

bool Value::is_consteval() const {
    return accept([&](const auto& val) { return val.is_consteval(); });
}

void Value::set_is_consteval(bool is_consteval) {
    accept([&](auto& val) { val.set_is_consteval(is_consteval); });
}

void Value::with_rvalue(std::function<void(const RValue&)> cb) const {
    accept(
        [&](const LValue& lval) { lval.with_rvalue(cb); },
        [&](const RValue& rval) { cb(rval); });
}

} // namespace ulam

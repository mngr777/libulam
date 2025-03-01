#include <libulam/detail/variant.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() const {
    return accept(
        [&](Ref<const Var> var) { return var->type(); },
        [&](const ArrayAccess& array_access) { return array_access.type(); },
        [&](const ObjectView& obj_view) { return obj_view.type(); },
        [&](const BoundProp& bound_prop) { return bound_prop.mem()->type(); },
        [&](const BoundFunSet& bound_fset) -> Ref<Type> { assert(false); },
        [&](const std::monostate&) -> Ref<Type> { assert(false); });
}

ObjectView LValue::obj_view() {
    return accept(
        [&](Ref<Var> var) { return var->obj_view(); },
        [&](ArrayAccess& array_access) {
            return array_access.item_object_view();
        },
        [&](ObjectView& obj_view) { return ObjectView{obj_view}; },
        [&](BoundProp& bound_prop) { return bound_prop.mem_obj_view(); },
        [&](BoundFunSet&) -> ObjectView { assert(false); },
        [&](std::monostate&) -> ObjectView { assert(false); });
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<const Var> var) { return var->rvalue(); },
        [&](const ArrayAccess& array_access) { return array_access.load(); },
        [&](const ObjectView& obj_view) { return RValue{obj_view.copy()}; },
        [&](const BoundProp& bound_prop) { return bound_prop.load(); },
        [&](const BoundFunSet&) -> RValue { assert(false); },
        [&](const std::monostate&) -> RValue { assert(false); });
}

LValue LValue::array_access(Ref<Type> item_type, array_idx_t index) {
    return derived(accept(
        [&](Ref<Var> var) {
            return ArrayAccess{var->array_view(), item_type, index};
        },
        [&](ArrayAccess& array_access) {
            return array_access.item_array_access(index);
        },
        [&](BoundProp& bound_prop) {
            return ArrayAccess{bound_prop.mem_array_view(), item_type, index};
        },
        [&](auto& other) -> ArrayAccess { assert(false); }));
}

LValue LValue::bound_prop(Ref<Prop> prop) {
    return derived(accept(
        [&](Ref<Var> var) { return BoundProp{var->obj_view(), prop}; },
        [&](ArrayAccess& array_access) {
            auto obj_view = array_access.item_object_view();
            return BoundProp{obj_view, prop};
        },
        [&](ObjectView& obj_view) { return BoundProp{obj_view, prop}; },
        [&](BoundProp& bound_prop) {
            return bound_prop.mem_obj_bound_prop(prop);
        },
        [&](auto& other) -> BoundProp { assert(false); }));
}

LValue LValue::bound_fset(Ref<FunSet> fset) {
    return derived(accept(
        [&](Ref<Var> var) { return BoundFunSet{var->obj_view(), fset}; },
        [&](ArrayAccess& array_access) {
            auto obj_view = array_access.item_object_view();
            return BoundFunSet{obj_view, fset};
        },
        [&](ObjectView& obj_view) { return BoundFunSet{obj_view, fset}; },
        [&](BoundProp& bound_prop) {
            return bound_prop.mem_obj_bound_fset(fset);
        },
        [&](auto& other) -> BoundFunSet { assert(false); }));
}

LValue LValue::bound_self() {
    auto lval = derived(accept(
        [&](BoundFunSet& bound_fset) { return bound_fset.obj_view(); },
        [&](BoundProp& bound_prop) { return bound_prop.obj_view(); },
        [&](auto& other) -> ObjectView { assert(false); }));
    lval.set_is_xvalue(false);
    return lval;
}

LValue LValue::self() {
    auto lval = derived(obj_view());
    lval.set_is_xvalue(false);
    return lval;
}

Value LValue::assign(RValue&& rval) {
    return accept(
        [&](Ref<Var> var) {
            var->set_value(Value{std::move(rval)});
            return Value{LValue{var}};
        },
        [&](ArrayAccess& array_access) {
            array_access.store(rval.copy()); // TODO: try to avoid copying
            return Value{std::move(rval)};
        },
        [&](BoundProp& bound_prop) {
            bound_prop.store(std::move(rval));
            return Value{LValue{bound_prop}};
        },
        [&](auto& other) -> Value { assert(false); });
}

// RValue

RValue RValue::copy() const {
    return accept(
        [&](const Bits& bits) { return RValue{bits.copy(), is_consteval()}; },
        [&](const Array& array) {
            return RValue{array.copy(), is_consteval()};
        },
        [&](SPtr<const Object> obj) {
            return RValue{obj->copy(), is_consteval()};
        },
        [&](auto value) { return RValue{value, is_consteval()}; });
}

Ref<Type> RValue::obj_type() const {
    return accept(
        [&](SPtr<Object> obj) { return obj->type(); },
        [&](auto& other) -> Ref<Type> { assert(false); });
}

ObjectView RValue::obj_view() {
    return accept(
        [&](SPtr<Object> obj) { return obj->view(); },
        [&](auto& other) -> ObjectView { assert(false); });
}

RValue RValue::array_access(Ref<Type> item_type, array_idx_t index) {
    return accept(
        [&](const Array& array) { return array.load(item_type, index); },
        [&](auto& other) -> RValue { assert(false); });
}

LValue RValue::bound_prop(Ref<Prop> prop) {
    return accept(
        [&](SPtr<Object> obj) {
            assert(obj->cls());
            assert(
                prop->cls() == obj->cls() ||
                prop->cls()->is_base_of(obj->cls()));
            return LValue{BoundProp{obj, prop}};
        },
        [&](auto& other) -> LValue { assert(false); });
}

LValue RValue::bound_fset(Ref<FunSet> fset) {
    return accept(
        [&](SPtr<Object> obj) {
            assert(
                fset->cls() == obj->cls() ||
                fset->cls()->is_base_of(obj->cls()));
            return LValue{BoundFunSet{obj, fset}};
        },
        [&](auto& other) -> LValue { assert(false); });
}

LValue RValue::self() {
    LValue lval{obj_view()};
    lval.set_scope_lvl(AutoScopeLvl);
    lval.set_is_xvalue(false);
    return lval;
}

// Value

bool Value::empty() const {
    return accept([&](auto& val) { return val.empty(); });
}

Ref<Type> Value::obj_type() const {
    return accept(
        [&](const LValue& lval) {
            auto type = lval.type();
            return type->canon()->is_object() ? type : Ref<Type>{};
        },
        [&](const RValue& rval) {
            return rval.is<SPtr<Object>>() ? rval.get<SPtr<Object>>()->type()
                                           : Ref<Type>{};
        });
}

ObjectView Value::obj_view() {
    return accept([&](auto& val) { return val.obj_view(); });
}

Value Value::array_access(Ref<Type> item_type, array_idx_t index) {
    return accept(
        [&](auto& val) { return Value{val.array_access(item_type, index)}; });
}

Value Value::bound_prop(Ref<Prop> prop) {
    return accept([&](auto& val) { return Value{val.bound_prop(prop)}; });
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

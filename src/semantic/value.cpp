#include "libulam/semantic/value/array.hpp"
#include "libulam/semantic/value/bound.hpp"
#include <libulam/detail/variant.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() {
    return accept(
        [&](Ref<Var> var) { return var->type(); },
        [&](ArrayAccess& array_access) { return array_access.type(); },
        [&](ObjectView& obj_view) -> Ref<Type> { return obj_view.cls(); },
        [&](BoundProp& bound_prop) { return bound_prop.mem()->type(); },
        [&](auto& other) -> Ref<Type> { assert(false); });
}

Ref<Class> LValue::obj_cls() {
    auto type_ = type();
    assert(type_->is_class());
    return type_->as_class();
}

ObjectView LValue::obj_view() {
    return accept(
        [&](Ref<Var> var) { return var->obj_view(); },
        [&](ArrayAccess& array_access) {
            return array_access.item_object_view();
        },
        [&](ObjectView& obj_view) { return ObjectView{obj_view}; },
        [&](BoundProp& bound_prop) { return bound_prop.mem_obj_view(); },
        [&](auto& other) -> ObjectView { assert(false); });
}

RValue LValue::rvalue() const {
    return accept(
        [&](Ref<Var> var) { return var->rvalue(); },
        [&](const ArrayAccess& array_access) { return array_access.load(); },
        [&](const ObjectView& obj_view) { return RValue{obj_view.copy()}; },
        [&](const BoundProp& bound_prop) { return bound_prop.load(); },
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

LValue LValue::bound_prop(Ref<Prop> prop) {
    return LValue{accept(
        [&](Ref<Var> var) { return BoundProp{var->obj_view(), prop}; },
        [&](ArrayAccess& array_access) {
            auto obj_view = array_access.item_object_view();
            return BoundProp{obj_view, prop};
        },
        [&](ObjectView& obj_view) { return BoundProp{obj_view, prop}; },
        [&](BoundProp& bound_prop) {
            return bound_prop.mem_obj_bound_prop(prop);
        },
        [&](auto& other) -> BoundProp { assert(false); })};
}

LValue LValue::bound_fset(Ref<FunSet> fset) {
    return LValue{accept(
        [&](Ref<Var> var) { return BoundFunSet{var->obj_view(), fset}; },
        [&](ArrayAccess& array_access) {
            auto obj_view = array_access.item_object_view();
            return BoundFunSet{obj_view, fset};
        },
        [&](ObjectView& obj_view) { return BoundFunSet{obj_view, fset}; },
        [&](BoundProp& bound_prop) {
            return bound_prop.mem_obj_bound_fset(fset);
        },
        [&](auto& other) -> BoundFunSet { assert(false); })};
}

Value LValue::assign(RValue&& rval) {
    return accept(
        [&](Ref<Var> var) {
            var->set_value(Value{std::move(rval)});
            return Value{LValue{var}};
        },
        [&](ArrayAccess& array_access) {
            array_access.store(rval.copy()); // TODO: try avoiding copying
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
        [&](const Atom& atom) { return RValue{atom.copy(), is_consteval()}; },
        [&](const Array& array) {
            return RValue{array.copy(), is_consteval()};
        },
        [&](SPtr<const Object> obj) {
            return RValue{obj->copy(), is_consteval()};
        },
        [&](auto value) { return RValue{value, is_consteval()}; });
}

Ref<Class> RValue::obj_cls() {
    return accept(
        [&](SPtr<Object> obj) { return obj->cls(); },
        [&](auto& other) -> Ref<Class> { assert(false); });
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

// Value

Ref<Class> Value::obj_cls() {
    return accept(
        [&](LValue& lval) { return lval.obj_cls(); },
        [&](RValue& rval) { return rval.obj_cls(); },
        [&](auto& other) -> Ref<Class> { assert(false); });
}

ObjectView Value::obj_view() {
    return accept(
        [&](LValue& lval) { return lval.obj_view(); },
        [&](RValue& rval) { return rval.obj_view(); },
        [&](auto& other) -> ObjectView { assert(false); });
}

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

Value Value::bound_prop(Ref<Prop> prop) {
    return accept(
        [&](LValue& lval) { return Value{lval.bound_prop(prop)}; },
        [&](RValue& rval) { return Value{rval.bound_prop(prop)}; },
        [&](auto& other) -> Value { assert(false); });
}

Value Value::bound_fset(Ref<FunSet> fset) {
    return accept(
        [&](LValue& lval) { return Value{lval.bound_fset(fset)}; },
        [&](RValue& rval) { return Value{rval.bound_fset(fset)}; },
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

bool Value::is_consteval() const {
    return accept(
        [&](const LValue& lval) { return lval.is_consteval(); },
        [&](const RValue& rval) { return rval.is_consteval(); },
        [&](const std::monostate& val) { return false; });
}

void Value::with_rvalue(std::function<void(const RValue&)> cb) const {
    accept(
        [&](const LValue& lval) {
            // TODO: LValue::with_rvalue
            auto rval = lval.rvalue();
            cb(rval);
        },
        [&](const RValue& rval) { cb(rval); },
        [&](const std::monostate&) {
            RValue rval;
            cb(rval);
        });
}

} // namespace ulam

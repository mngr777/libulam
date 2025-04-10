#include <cassert>
#include <libulam/semantic/type/ops.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/typed_value.hpp>

namespace ulam {

namespace {

bool is_shift(Op op) {
    switch (op) {
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::ShiftLeft:
    case Op::ShiftRight:
        return true;
    default:
        return false;
    }
}

bool is_numeric(Ref<const PrimType> type) {
    return type->is(IntId) || type->is(UnsignedId) || type->is(UnaryId);
}

TypeError check_type_match(Ref<const Type> type, BuiltinTypeId target) {
    type = type->actual();
    if (type->is_prim() && type->as_prim()->is(target))
        return {TypeError::Ok};
    if (type->is_impl_castable_to(target))
        return {TypeError::ImplCastRequired, target};
    if (type->is_expl_castable_to(target))
        return {TypeError::ExplCastRequired, target};
    return {TypeError::Incompatible};
}

TypeError
check_type_match(Ref<const Type> type, const Value& val, BuiltinTypeId target) {
    type = type->actual();
    if (type->is_prim() && type->as_prim()->is(target))
        return {TypeError::Ok};
    if (type->is_impl_castable_to(target, val))
        return {TypeError::ImplCastRequired, target};
    if (type->is_expl_castable_to(target))
        return {TypeError::ExplCastRequired, target};
    return {TypeError::Incompatible};
}

TypeError
check_type_match(Ref<const Type> type, const Value& val, Ref<Type> target) {
    type = type->actual();
    if (type->is_same_actual(target))
        return {TypeError::Ok};
    if (type->is_impl_castable_to(target, val))
        return {TypeError::ImplCastRequired, target};
    if (type->is_expl_castable_to(target))
        return {TypeError::ExplCastRequired, target};
    return {TypeError::Incompatible};
}

TypeErrorPair numeric_prim_binary_op_type_check_prim(
    Op op,
    Ref<const PrimType> l_type,
    Ref<const PrimType> r_type,
    const Value& r_val) {
    assert(is_numeric(r_type));
    TypeErrorPair errors;

    auto suggest = [&](BuiltinTypeId target) -> bool {
        if (l_type->is(target)) {
            errors.second = check_type_match(r_type, r_val, target);
            return true;
        } else if (r_type->is(target)) {
            errors.first = check_type_match(l_type, target);
            return true;
        }
        return false;
    };

    if (is_numeric(l_type) || is_numeric(l_type)) {
        // same type?
        if (l_type->is(r_type->bi_type_id())) {
            // suggest casting Unary to Unsigned
            if (l_type->is(UnaryId)) {
                errors.first = check_type_match(l_type, UnsignedId);
                errors.second = check_type_match(r_type, r_val, UnsignedId);
            }
            return errors;
        }

        // suggest casting to Unsigned for assignment ops if left is Unsigned
        if (ops::is_assign(op) && l_type->is(UnsignedId) && suggest(UnsignedId))
            return errors;

        // suggest casting to Int or Unsigned
        if (suggest(IntId) || suggest(UnsignedId))
            return errors;
        assert(false);

    } else {
        // suggest casting to Int
        errors.first = check_type_match(l_type, IntId);
        errors.second = check_type_match(r_type, r_val, IntId);
    }
    return errors;
}

TypeErrorPair numeric_prim_binary_op_type_check_class(
    Op op, Ref<PrimType> left, Ref<Class> right) {
    assert(is_numeric(left));
    TypeErrorPair errors;
    if (left->is(IntId)) {
        errors.second = check_type_match(right, IntId);
    } else {
        if (left->is(UnaryId))
            errors.first = check_type_match(left, UnsignedId);
        errors.second = check_type_match(right, UnsignedId);
    }
    return errors;
}

TypeErrorPair
prim_binary_op_type_check(Op op, Ref<PrimType> l_type, const TypedValue& r_tv) {
    auto r_type = r_tv.type()->actual();
    TypeErrorPair errors;
    switch (ops::kind(op)) {
    case ops::Kind::Assign: {
        errors.second = check_type_match(r_tv.type(), r_tv.value(), l_type);
    } break;
    case ops::Kind::Equality: {
        errors.second =
            check_type_match(r_tv.type(), r_tv.value(), l_type->bi_type_id());
    } break;
    case ops::Kind::Comparison:
    case ops::Kind::Numeric: {
        auto r_type = r_tv.type()->actual();
        if (r_type->is_prim())
            return numeric_prim_binary_op_type_check_prim(
                op, l_type, r_type->as_prim(), r_tv.value());
        if (r_type->is_class())
            return numeric_prim_binary_op_type_check_class(
                op, l_type, r_type->as_class());
        errors.first = {TypeError::Incompatible};
        errors.second = {TypeError::Incompatible};
    } break;
    case ops::Kind::Logical: {
        errors.first = check_type_match(l_type, BoolId);
        errors.second = check_type_match(r_type, BoolId);
    } break;
    case ops::Kind::Bitwise: {
        errors.first = check_type_match(l_type, BitsId);
        errors.second = check_type_match(
            r_type, r_tv.value(), is_shift(op) ? UnsignedId : BitsId);
    } break;
    default:
        errors.first = {TypeError::Incompatible};
        errors.second = {TypeError::Incompatible};
    }
    return errors;
}

TypeErrorPair
class_binary_op_type_check(Op op, Ref<Class> cls, Ref<const Type> r_type) {
    assert(r_type->is_actual());
    TypeErrorPair errors;
    if (cls->has_op(op))
        return errors;

    if (op == Op::Assign) {
        errors.second = check_type_match(r_type, Value{}, cls);
        return errors;
    }

    errors.first.status = TypeError::Incompatible;
    errors.second.status = TypeError::Incompatible;
    return errors;
}

TypeErrorPair
atom_binary_op_type_check(Op op, Ref<Type> type, Ref<const Type> r_type) {
    assert(type->is(AtomId));
    TypeErrorPair errors;
    switch (op) {
    case Op::Assign:
        errors.second = check_type_match(r_type, Value{}, type);
        return errors;
    default:
        errors.first.status = TypeError::Incompatible;
        errors.second.status = TypeError::Incompatible;
        return errors;
    }
}

} // namespace

TypeError::TypeError(Status status): status{status} {
    assert(status == Ok || status == Incompatible);
}

TypeError::TypeError(Status status, Ref<Type> cast_type):
    status{status}, cast_type{cast_type} {
    assert(status == ImplCastRequired || status == ExplCastRequired);
}

TypeError::TypeError(Status status, BuiltinTypeId cast_bi_type_id):
    status{status}, cast_bi_type_id{cast_bi_type_id} {
    assert(status == ImplCastRequired || status == ExplCastRequired);
    assert(is_prim(cast_bi_type_id));
}

TypeError unary_op_type_check(Op op, Ref<Type> type) {
    type = type->actual();

    // overloaded?
    if (type->is_class()) {
        if (type->as_class()->has_op(op))
            return {TypeError::Ok};
    }

    switch (ops::kind(op)) {
    case ops::Kind::Numeric:
        if (!type->is_prim() || !is_numeric(type->as_prim()))
            return {TypeError::Incompatible};
        if (type->as_prim()->is(UnaryId))
            return check_type_match(type->as_prim(), UnaryId);
        return {TypeError::Ok};
    case ops::Kind::Bitwise:
        return check_type_match(type, BitsId);
    case ops::Kind::Logical:
        return check_type_match(type, BoolId);
    case ops::Kind::Objective:
        if (!type->is_object())
            return {TypeError::Incompatible};
        return {};
    default:
        assert(false);
    }
}

TypeErrorPair
binary_op_type_check(Op op, Ref<Type> l_type, const TypedValue& r_tv) {
    auto l_actual = l_type->actual();
    if (l_actual->is_prim())
        return prim_binary_op_type_check(op, l_actual->as_prim(), r_tv);

    if (l_actual->is_class())
        return class_binary_op_type_check(
            op, l_actual->as_class(), r_tv.type()->actual());

    if (l_actual->is(AtomId))
        return atom_binary_op_type_check(op, l_actual, r_tv.type()->deref());

    TypeErrorPair errors;
    if (op == Op::Assign && l_type->is_same(r_tv.type()))
        return errors;
    errors.first.status = TypeError::Incompatible;
    errors.second.status = TypeError::Incompatible;
    return errors;
}

} // namespace ulam

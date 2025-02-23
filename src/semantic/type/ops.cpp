#include "libulam/semantic/type/builtin_type_id.hpp"
#include <cassert>
#include <libulam/semantic/type/ops.hpp>
#include <libulam/semantic/type/prim.hpp>

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
    assert(type->is_canon());
    if (type->is_prim() && type->as_prim()->is(target))
        return {TypeError::Ok};
    if (type->is_impl_castable_to(target))
        return {TypeError::ImplCastRequired, target};
    if (type->is_expl_castable_to(target))
        return {TypeError::ExplCastRequired, target};
    return {TypeError::Incompatible};
}

TypeErrorPair numeric_prim_binary_op_type_check_prim(
    Op op, Ref<const PrimType> left, Ref<const PrimType> right) {
    assert(is_numeric(left));
    TypeErrorPair errors;

    auto suggest = [&](BuiltinTypeId target) -> bool {
        if (left->is(target)) {
            errors.second = check_type_match(right, target);
            return true;
        } else if (right->is(target)) {
            errors.first = check_type_match(left, target);
            return true;
        }
        return false;
    };

    if (is_numeric(left) || is_numeric(right)) {
        // same type?
        if (left->is(right->builtin_type_id())) {
            // suggest casting Unary ot Unsigned
            if (left->is(UnaryId)) {
                errors.first = check_type_match(left, UnsignedId);
                errors.second = check_type_match(right, UnsignedId);
            }
            return errors;
        }
        // suggest casting to Int or Unsigned
        if (suggest(IntId) || suggest(UnsignedId))
            return errors;
        assert(false);

    } else {
        // suggest casting to Int
        errors.first = check_type_match(left, IntId);
        errors.second = check_type_match(right, IntId);
    }
    return errors;
}

TypeErrorPair numeric_prim_binary_op_type_check_class(
    Op op, Ref<const PrimType> left, Ref<const Class> right) {
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

TypeErrorPair prim_binary_op_type_check(
    Op op, Ref<const PrimType> left, Ref<const Type> right) {
    assert(right->canon() == right);
    TypeErrorPair errors;
    switch (ops::kind(op)) {
    case ops::Kind::Assign: {
        errors.second = check_type_match(right, left->builtin_type_id());
    } break;
    case ops::Kind::Equality: {
        errors.second = check_type_match(right, left->builtin_type_id());
    } break;
    case ops::Kind::Numeric: {
        if (right->is_prim())
            return numeric_prim_binary_op_type_check_prim(
                op, left, right->as_prim());
        if (right->is_class())
            return numeric_prim_binary_op_type_check_class(
                op, left, right->as_class());
        errors.first = {TypeError::Incompatible};
        errors.second = {TypeError::Incompatible};
    } break;
    case ops::Kind::Logical: {
        errors.first = check_type_match(left, BoolId);
        errors.second = check_type_match(right, BoolId);
    } break;
    case ops::Kind::Bitwise: {
        errors.first = check_type_match(left, BitsId);
        errors.second =
            check_type_match(right, is_shift(op) ? UnsignedId : BitsId);
    } break;
    }
    return errors;
}

TypeErrorPair class_binary_op_type_check(
    Op op, Ref<const Class> left, Ref<const Type> right) {
    assert(right->canon() == right);
    // TODO: class operators
    TypeErrorPair errors;
    if (op == Op::Assign && left->is_same(right))
        return errors;
    errors.first.status = TypeError::Incompatible;
    errors.second.status = TypeError::Incompatible;
    return errors;
}

} // namespace

TypeError::TypeError(Status status): status{status} {
    assert(status == Ok || status == Incompatible);
}

TypeError::TypeError(Status status, BuiltinTypeId cast_bi_type_id):
    status(status), cast_bi_type_id(cast_bi_type_id) {
    assert(status == ImplCastRequired || status == ExplCastRequired);
    assert(is_prim(cast_bi_type_id));
}

TypeError unary_op_type_check(Op op, Ref<const Type> type) {
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
    default:
        assert(false);
    }
}

TypeErrorPair
binary_op_type_check(Op op, Ref<const Type> left, Ref<const Type> right) {
    if (left->canon()->is_prim())
        return prim_binary_op_type_check(
            op, left->canon()->as_prim(), right->canon());
    if (left->canon()->is_class())
        return class_binary_op_type_check(
            op, left->canon()->as_class(), right->canon());

    TypeErrorPair errors;
    if (op == Op::Assign && left->is_same(right))
        return errors;
    errors.first.status = TypeError::Incompatible;
    errors.second.status = TypeError::Incompatible;
    return errors;
}

} // namespace ulam

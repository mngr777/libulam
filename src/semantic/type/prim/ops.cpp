#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/prim/ops.hpp>

namespace ulam {

namespace {

bool is_numeric(Ref<const PrimType> type) {
    return type->is(IntId) || type->is(UnsignedId) || type->is(UnaryId);
}

PrimTypeError check_type_match(Ref<const Type> type, BuiltinTypeId target) {
    auto canon = type->canon();
    if (canon->is_prim() && canon->as_prim()->is(target))
        return {PrimTypeError::Ok, VoidId};
    if (canon->is_impl_castable_to(target))
        return {PrimTypeError::ImplCastRequired, target};
    if (canon->is_expl_castable_to(target))
        return {PrimTypeError::ExplCastRequired, target};
    return {PrimTypeError::Incompatible, VoidId};
}

} // Namespace

PrimTypeError prim_unary_op_type_check(Op op, Ref<PrimType> type) {
    switch (ops::kind(op)) {
    case ops::Kind::Numeric:
        if (!is_numeric(type))
            // NOTE: using IntId as placeholder for "any numeric type"
            return check_type_match(type, IntId);
        return {PrimTypeError::Ok, VoidId};
    case ops::Kind::Bitwise:
        return check_type_match(type, BitsId);
    case ops::Kind::Logical:
        return check_type_match(type, BoolId);
    default:
        assert(false);
    }
}

PrimTypeErrorPair prim_binary_op_type_check(
    Op op, Ref<PrimType> left_type, Ref<Type> right_type) {

    PrimTypeErrorPair errors;
    switch (ops::kind(op)) {
    case ops::Kind::Assign: {
        errors.second =
            check_type_match(right_type, left_type->builtin_type_id());
    } break;
    case ops::Kind::Equality: {
        errors.second =
            check_type_match(right_type, left_type->builtin_type_id());
    } break;
    case ops::Kind::Numeric: {
        auto suggest_cast = [&](BuiltinTypeId type_id) -> bool {
            assert(right_type->is_prim());
            if (left_type->is(type_id)) {
                errors.second = check_type_match(right_type, type_id);
                return true;
            } else if (right_type->as_prim()->is(type_id)) {
                errors.first = check_type_match(left_type, type_id);
                return true;
            }
            return false;
        };
        if (right_type->is_prim() &&
            (is_numeric(left_type) || is_numeric(right_type->as_prim()))) {
            // same type?
            if (left_type->builtin_type_id() == right_type->builtin_type_id()) {
                // Unary casted to Unsigned
                if (left_type->is(UnaryId)) {
                    errors.first = check_type_match(left_type, UnsignedId);
                    errors.second = check_type_match(left_type, UnsignedId);
                }
                return errors;
            }
            // suggest casting to preferred type, Int over Unsigned
            if (suggest_cast(IntId) || suggest_cast(UnsignedId))
                return errors;
            assert(false);

        } else {
            // suggest casting to Int
            errors.first = check_type_match(left_type, IntId);
            errors.second = check_type_match(right_type, IntId);
        }
    } break;
    case ops::Kind::Logical: {
        errors.first = check_type_match(left_type, BoolId);
        errors.second = check_type_match(right_type, BoolId);
    } break;
    case ops::Kind::Bitwise: {
        errors.first = check_type_match(left_type, BitsId);
        errors.second = check_type_match(right_type, BitsId);
    } break;
    }
    return errors;
}

} // namespace ulam

#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/prim/ops.hpp>

namespace ulam {

namespace {

bool is_numeric(Ref<const PrimType> type) {
    return type->is(IntId) || type->is(UnsignedId) || type->is(UnaryId);
}

PrimTypeError check_type_match(Ref<const PrimType> type, BuiltinTypeId target) {
    assert(target != VoidId);
    if (type->is(target))
        return {PrimTypeError::Ok, VoidId};
    if (type->is_impl_castable_to(target))
        return {PrimTypeError::ImplCastRequired, target};
    if (type->is_expl_castable_to(target))
        return {PrimTypeError::ExplCastRequired, target};
    return {PrimTypeError::Incompatible, VoidId};
}

} // Namespace

PrimTypeErrorPair prim_binary_op_type_check(
    Op op, Ref<PrimType> left_type, Ref<PrimType> right_type) {

    PrimTypeErrorPair errors;
    switch (ops::kind(op)) {
    case ops::Kind::Assign: {
        errors.second = check_type_match(right_type, left_type->builtin_type_id());
    } break;
    case ops::Kind::Numeric: {
        // one or both operands non-numeric?
        if (!is_numeric(left_type) || !is_numeric(right_type)) {
            // suggest casting to Int
            errors.first = check_type_match(left_type, IntId);
            errors.second = check_type_match(right_type, IntId);
        }

        // same type?
        if (left_type->builtin_type_id() == right_type->builtin_type_id())
            return errors;

        // Int wins, otherwise Unary becomes Unsigned (implicit)
        if (left_type->is(IntId) || right_type->is(IntId)) {
            auto& error = left_type->is(IntId) ? errors.second : errors.first;
            error = {PrimTypeError::ImplCastRequired, IntId};

        } else if (left_type->is(UnsignedId) || right_type->is(UnsignedId)) {
            auto& error =
                left_type->is(UnsignedId) ? errors.second : errors.first;
            error = {PrimTypeError::ImplCastRequired, UnsignedId};
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

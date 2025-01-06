#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/prim/ops.hpp>

namespace ulam {

namespace {

bool is_numeric(Ref<const PrimType> type) {
    return type->is(IntId) || type->is(UnsignedId);
}

PrimTypeError
check_type_match(Ref<const PrimType> type, BuiltinTypeId target) {
    if (type->is(target))
        return {PrimTypeError::Ok, VoidId};
    if (type->is_impl_castable(target))
        return {PrimTypeError::ImplCastRequired};
    if (type->is_expl_castable(target))
        return {PrimTypeError::ExplCastRequired, target};
    return {PrimTypeError::Incompatible, VoidId};
}

} // Namespace

PrimTypeErrorPair prim_binary_op_type_check(
    Op op, Ref<PrimType> left_type, Ref<PrimType> right_type) {

    PrimTypeErrorPair errors;
    switch (ops::kind(op)) {
    case ops::Kind::Numeric: {
        // one or both operands non-numeric?
        if (!is_numeric(left_type) || !is_numeric(right_type)) {
            // suggest casting to Int
            errors.first = check_type_match(left_type, IntId);
            errors.second = check_type_match(right_type, IntId);
        }

        // signed type wins
        if (left_type->builtin_type_id() != left_type->builtin_type_id()) {
            auto& error = left_type->is(IntId) ? errors.second : errors.first;
            error = {PrimTypeError::ExplCastRequired, IntId};
        }
        return errors;
    }
    case ops::Kind::Logical: {
        errors.first = check_type_match(left_type, BoolId);
        errors.second = check_type_match(right_type, BoolId);
    }
    case ops::Kind::Bitwise:
        errors.first = check_type_match(left_type, BitsId);
        errors.first = check_type_match(right_type, BoolId);
    }
    return errors;
}

} // namespace ulam

#include "libulam/semantic/expr_res.hpp"
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/prim/ops.hpp>

namespace ulam {

namespace {

PrimBinaryOpRes binary_op_numeric(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val);

PrimBinaryOpRes binary_op_logical(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val);

PrimBinaryOpRes binary_op_bitwise(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val);

bool is_numeric(Ref<const PrimType> type) {
    return type->is(IntId) || type->is(UnsignedId);
}

PrimOpTypeError make_type_error(bool is_castable, BuiltinTypeId required) {
    if (is_castable)
        return {required, PrimOpTypeError::CastSuggested};
    return {VoidId, PrimOpTypeError::Incompatible};
}

PrimOpTypeError make_type_error(Ref<PrimType> operand, BuiltinTypeId required) {
    if (operand->is(required))
        return {VoidId, PrimOpTypeError::Ok};
    return make_type_error(operand->is_castable(required), required);
}

PrimBinaryOpRes make_binary_single_type_error(
    int error_idx, Ref<PrimType> operand, BuiltinTypeId required) {
    assert(error_idx == 1 || error_idx == 2);
    auto error = make_type_error(operand, required);
    ExprError expr_error = (error.error == PrimOpTypeError::CastSuggested)
                               ? ExprError::CastRequired
                               : ExprError::InvalidOperandType;
    PrimBinaryOpRes res{expr_error};
    res.errors[error_idx] = error;
    return res;
}

ExprError binary_op_expr_error(
    PrimOpTypeError::Error left_error, PrimOpTypeError::Error right_error) {
    switch (std::max(left_error, right_error)) {
    case PrimOpTypeError::Error::Ok:
        return ExprError::Ok;
    case PrimOpTypeError::Error::CastSuggested:
        return ExprError::CastRequired;
    case PrimOpTypeError::Incompatible:
        return ExprError::InvalidOperandType;
    }
}

} // namespace

// PrimOpResBase

ExprRes PrimOpResBase::move_res() {
    ExprRes res;
    std::swap(res, _res);
    return res;
}

// PrimBinaryOpRes

PrimBinaryOpRes::PrimBinaryOpRes(
    PrimOpTypeError left_error, PrimOpTypeError right_error):
    PrimBinaryOpRes{binary_op_expr_error(left_error.error, right_error.error)} {
    assert(
        !left_error.ok() || !right_error.ok()); // somehting must be not right
}

PrimBinaryOpRes binary_op(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val) {
    assert(!left_type->is(VoidId) && !right_type->is(VoidId));

    switch (ops::kind(op)) {
    case ops::Kind::Numeric:
        return binary_op_numeric(
            op, left_type, left_val, right_type, right_val);
    case ops::Kind::Logical:
        return binary_op_logical(
            op, left_type, left_val, right_type, right_val);
    case ops::Kind::Bitwise:
        return binary_op_bitwise(
            op, left_type, left_val, right_type, right_val);
    }
}

namespace {

PrimBinaryOpRes binary_op_numeric(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val) {

    // is applicable to operands?
    if (!is_numeric(left_type) || !is_numeric(right_type)) {
        // if one is numeric, suggest casting other operand if possible
        if (is_numeric(left_type)) {
            return make_binary_single_type_error(
                1, right_type, left_type->builtin_type_id());

        } else if (is_numeric(right_type)) {
            return make_binary_single_type_error(
                0, left_type, right_type->builtin_type_id());
        }

        // both are non-numeric
        PrimBinaryOpRes res{ExprError::InvalidOperandType};
        res.errors[0] = {VoidId, PrimOpTypeError::Incompatible};
        res.errors[1] = {VoidId, PrimOpTypeError::Incompatible};
        return res;
    }

    // explicit cast required?
    // (signed type wins)
    if (left_type->builtin_type_id() != right_type->builtin_type_id()) {
        PrimBinaryOpRes res{ExprError::CastRequired};
        PrimOpTypeError error{IntId, PrimOpTypeError::CastSuggested};
        if (left_type->is(UnsignedId)) {
            assert(right_type->is(IntId));
            res.errors[0] = error;
        } else {
            assert(left_type->is(IntId) && right_type->is(UnsignedId));
            res.errors[1] = error;
        }
        return res;
    }

    // perform operation
    return {left_type->binary_op(op, left_val, right_type, right_val)};
}

PrimBinaryOpRes binary_op_logical(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val) {

    auto left_error = make_type_error(left_type, BoolId);
    auto right_error = make_type_error(right_type, BoolId);
    if (!left_error.ok() || !right_error.ok())
        return {left_error, right_error};

    return {left_type->binary_op(op, left_val, right_type, right_val)};
}

PrimBinaryOpRes binary_op_bitwise(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val) {

    auto left_error = make_type_error(left_type, BitsId);
    auto right_error = make_type_error(right_type, BitsId);
    if (!left_error.ok() || !right_error.ok())
        return {left_error, right_error};

    return {left_type->binary_op(op, left_val, right_type, right_val)};
}

} // namespace

} // namespace ulam

#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/int.hpp>

namespace ulam {

bool IntType::is_convertible(Ref<const Type> type) {
    // is primitive?
    auto prim = type->as_prim();
    if (!prim)
        return false;
    switch (prim->builtin_type_id()) {
    case IntId:
        return prim->bitsize() <= bitsize();
    case UnsignedId:
        return prim->bitsize() < bitsize() ||
               prim->bitsize() == ULAM_MAX_INT_SIZE;
    case UnaryId:
        return detail::bitsize((Unsigned)prim->bitsize()) < bitsize();
    default:
        return false;
    }
}

// Value IntType::cast(
//     ast::Ref<ast::Node> node,
//     Ref<const Type> type,
//     const Value& value,
//     bool is_impl) {
//     // is basic type?
//     if (!type->is_basic())
//         return {};
//     auto prim = type->basic()->as_prim();
//     if (!prim)
//         return {};
//     switch (prim->builtin_type_id()) {
//     case IntId: {
//         // implicitly convertible?
//         if (is_impl && !is_convertible(type)) {
//             diag().emit(
//                 diag::Error, node->loc_id(), 1, "invalid implicit cast");
//         }
//         // is value known?
//         auto rval = value.rvalue();
//         if (rval->is_unknown())
//             return RValue{};
//         // truncate
//         assert(rval->is<Integer>());
//         Integer val = rval->get<Integer>();
//         Integer truncated = detail::truncate(val, bitsize());
//         if (truncated != val)
//             diag().emit(diag::Error, node->loc_id(), 1, "value truncated");
//         return RValue{truncated};
//     }
//     case UnsignedId: {
//         // implicitly convertible?
//         if (is_impl && !is_convertible(type)) {
//             diag().emit(
//                 diag::Error, node->loc_id(), 1, "invalid implicit cast");
//         }
//         // is value known?
//         auto rval = value.rvalue();
//         if (rval->is_unknown())
//             return RValue{};
//         // truncate
//         assert(rval->is<Unsigned>());
//         Unsigned val = rval->get<Unsigned>();
//         Unsigned truncated = detail::truncate(val, bitsize() - 1);
//         if (truncated != val)
//             diag().emit(diag::Error, node->loc_id(), 1, "value truncated");
//         return RValue{truncated};
//     }
//     case BoolId: {
//         if (is_impl) {
//             diag().emit(
//                 diag::Error, node->loc_id(), 1, "invalid implicit cast");
//         }
//         auto rval = value.rvalue();
//         return rval->is_unknown() ? RValue{}
//                                   : RValue{rval->get<Bool>() ? 1 : 0};
//     }
//     case UnaryId: {
//         // implicitly convertible?
//         if (is_impl && !is_convertible(type)) {
//             diag().emit(
//                 diag::Error, node->loc_id(), 1, "invalid implicit cast");
//         }
//         // is value known?
//         auto rval = value.rvalue();
//         if (rval->is_unknown())
//             return RValue{};
//         // truncate
//         assert(rval->is<Unsigned>());
//         Unsigned val = rval->get<Unsigned>();
//         Unsigned truncated = detail::truncate(val, bitsize());
//         if (truncated != val)
//             diag().emit(diag::Error, node->loc_id(), 1, "value truncated");
//         return RValue{truncated};
//     }
//         // TODO: BitsId
//     default:
//         // not convertible
//         break;
//     }
//     return {};
// }

// ExprRes IntType::binary_op(
//     ast::Ref<ast::BinaryOp> node,
//     Value& left,
//     Ref<const Type> right_type,
//     const Value& right) {
//     // - implicit cast to Int(n)
//     // - Int(a) op Int(n)
//     switch (right_type->builtin_type_id()) {
//     case IntId:
//         return binary_op_int(node, left, right_type, right);
//     case UnsignedId: {
//         // Unsigned(b) -> Int(min(MaxSize, b + 1))
//         auto right_int_size =
//             std::min(MaxSize, (bitsize_t)(right_type->bitsize() + 1));
//         auto right_int_type = prim_type(node, IntId, right_int_size);
//         auto right_int_val = right_int_type->cast(node, right_type, right);
//         assert(!right_int_val.is_nil());
//         // Int(a) + Int(min(MaxSize, b + 1))
//         return binary_op_int(node, left, right_int_type, right_int_val);
//     }
//     case BoolId: {
//         // Bool(b) -> Int(2) (NOTE: invalid implicit cast)
//         auto right_int_type = prim_type(node, IntId, 2);
//         auto right_int_val = right_int_type->cast(node, right_type, right);
//         assert(!right_int_val.is_nil());
//         // Int(a) + Int(2)
//         return binary_op_int(node, left, right_int_type, right_int_val);
//     }
//     case UnaryId: {
//         // Unary(b) -> Int(bitsize(b) + 1)
//         auto right_int_size = detail::bitsize((Integer)right_type->bitsize());
//         auto right_int_type = prim_type(node, IntId, right_int_size);
//         auto right_int_val = right_int_type->cast(node, right_int_type, right);
//         assert(!right_int_val.is_nil());
//         // Int(a) + Int(bitsize(b) + 1)
//         return binary_op_int(node, left, right_int_type, right_int_val);
//     }
//     case BitsId:
//         return {ExprError::CastRequired};
//     default:
//         return {ExprError::NoOperator};
//     }
// }

// ExprRes IntType::binary_op_int(
//     ast::Ref<ast::BinaryOp> node,
//     Value& left,
//     Ref<const Type> right_type,
//     const Value& right) {
//     // Int(a) op Int(b)
//     auto left_rval = left.rvalue();
//     auto right_rval = right.rvalue();
//     assert(left_rval->is_unknown() || left_rval->is<Integer>());
//     assert(right_rval->is_unknown() || right_rval->is<Integer>());
//     bool is_unknown = (left_rval->is_unknown() || right_rval->is_unknown());
//     switch (node->op()) {
//     case Op::Prod: {
//         auto size =
//             std::min(MaxSize, (bitsize_t)(bitsize() + right_type->bitsize()));
//         auto type = prim_type(node, IntId, size);
//         if (is_unknown)
//             return {type, RValue{}};
//         // prod
//         auto [val, is_truncated] = detail::safe_prod(
//             left_rval->get<Integer>(), right_rval->get<Integer>());
//         // truncated?
//         if (is_truncated)
//             diag().emit(diag::Warn, node->loc_id(), 1, "result is truncated");
//         return {type, RValue{val}};
//     }
//     case Op::Quot: {
//         // Int(a) / Int(b) = Int(min(MaxSize, a + 1)) NOTE: does not match
//         // ULAM's max(a, b), TODO: investigate
//         bitsize_t size = std::min(MaxSize, (bitsize_t)(bitsize() + 1));
//         auto type = prim_type(node, IntId, size);
//         if (is_unknown)
//             return {type, RValue{}};
//         auto right_val = right_rval->get<Integer>();
//         if (right_val == 0)
//             diag().emit(diag::Error, node->loc_id(), 1, "division by zero");
//         auto val = detail::safe_quot(left_rval->get<Integer>(), right_val);
//         return {type, RValue{val}};
//     }
//     case Op::Rem: {
//         // Int(a) % Int(b) = Int(a)
//         if (is_unknown)
//             return {this, RValue{}};
//         auto right_val = right_rval->get<Integer>();
//         if (right_val == 0)
//             diag().emit(diag::Error, node->loc_id(), 1, "division by zero");
//         auto val = detail::safe_rem(left_rval->get<Integer>(), right_val);
//         return {this, RValue{val}};
//     }
//     case Op::Sum: {
//         // Int(a) + Int(b) = Int(min(MaxSize, max(a, b) + 1))
//         bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
//         size = std::min(size, MaxSize);
//         auto type = prim_type(node, IntId, size);
//         if (is_unknown)
//             return {type, RValue{}};
//         // sum
//         auto [val, rest] = detail::safe_sum(
//             left_rval->get<Integer>(), right_rval->get<Integer>());
//         // truncated?
//         if (rest != 0)
//             diag().emit(diag::Warn, node->loc_id(), 1, "result is truncated");
//         return {type, RValue{val}};
//     }
//     case Op::Diff: {
//         // Int(a) - Int(b) = Int(min(MaxSize), max(a, b) + 1)
//         bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
//         size = std::min(size, MaxSize);
//         auto type = prim_type(node, IntId, size);
//         if (is_unknown)
//             return {type, RValue{}};
//         // diff
//         auto [val, rest] = detail::safe_diff(
//             left_rval->get<Integer>(), right_rval->get<Integer>());
//         // truncated?
//         if (rest != 0)
//             diag().emit(diag::Warn, node->loc_id(), 1, "result is truncated");
//         return {type, RValue{val}};
//     }
//     default:
//         return {ExprError::NotImplemented};
//     }
// }

} // namespace ulam

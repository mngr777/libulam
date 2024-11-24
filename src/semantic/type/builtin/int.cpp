#include <libulam/ast/nodes/expr.hpp>
#include <libulam/semantic/type/builtin/int.hpp>

namespace ulam {

ExprRes IntType::binary_op(
    ast::Ref<ast::BinaryOp> node, Value& lhs, Ref<Type> rhs_type, Value& rhs) {
    switch (rhs_type->builtin_type_id()) {
    case IntId:
    case UnsignedId:
    case BoolId:
    case UnaryId:
    default:
        return {ExprError::NoOperator};
    }
}

} // namespace ulam

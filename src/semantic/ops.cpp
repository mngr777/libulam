#include "libulam/semantic/ops.hpp"
#include <cassert>

namespace ulam::ops {

const char* str(Op op) {
#define OP(str, op)                                                            \
    case Op::op:                                                               \
        return str;
    switch (op) {
#include <libulam/semantic/ops.inc.hpp>
    default:
        assert(false);
    }
#undef OP
}

Kind kind(Op op) {
    switch (op) {
    case Op::Assign:
        return Kind::Assign;
    case Op::Equal:
    case Op::NotEqual:
        return Kind::Equality;
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
    case Op::Sum:
    case Op::Diff:
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
    case Op::PreInc:
    case Op::PreDec:
    case Op::PostInc:
    case Op::PostDec:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignSum:
    case Op::AssignDiff:
        return Kind::Numeric;
    case Op::Negate:
    case Op::And:
    case Op::Or:
        return Kind::Logical;
    case Op::ShiftLeft:
    case Op::ShiftRight:
    case Op::BwNot:
    case Op::BwAnd:
    case Op::BwXor:
    case Op::BwOr:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return Kind::Bitwise;
    default:
        assert(false);
    }
}

bool is_numeric(Op op) { return kind(op) == Kind::Numeric; }

bool is_logical(Op op) { return kind(op) == Kind::Logical; }

bool is_bitwise(Op op) { return kind(op) == Kind::Bitwise; }

bool is_assign(Op op) {
    switch (op) {
    case Op::Assign:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return true;
    default:
        return false;
    }
}

Op non_assign(Op op) {
    switch (op) {
    case Op::Assign:
        return Op::None;
    case Op::AssignProd:
        return Op::Prod;
    case Op::AssignQuot:
        return Op::Quot;
    case Op::AssignRem:
        return Op::Rem;
    case Op::AssignSum:
        return Op::Sum;
    case Op::AssignDiff:
        return Op::Diff;
    case Op::AssignShiftLeft:
        return Op::ShiftLeft;
    case Op::AssignShiftRight:
        return Op::ShiftRight;
    case Op::AssignBwAnd:
        return Op::BwAnd;
    case Op::AssignBwXor:
        return Op::BwXor;
    case Op::AssignBwOr:
        return Op::BwOr;
    default:
        assert(!is_assign(op));
        return op;
    }
}

Prec prec(Op op) {
    switch (op) {
    case Op::Is:
    case Op::As:
    case Op::FunCall:
    case Op::ArrayAccess:
    case Op::MemberAccess:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Negate:
    case Op::BwNot:
        return 13;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PostInc:
    case Op::PostDec:
        return 12;
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
        return 11;
    case Op::Sum:
    case Op::Diff:
        return 10;
    case Op::ShiftLeft:
    case Op::ShiftRight:
        return 9;
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
        return 8;
    case Op::Equal:
    case Op::NotEqual:
        return 7;
    case Op::BwAnd:
        return 6;
    case Op::BwXor:
        return 5;
    case Op::BwOr:
        return 4;
    case Op::And:
        return 3;
    case Op::Or:
        return 2;
    case Op::Assign:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return 1;
    case Op::Comma:
        return 0;
    default:
        return -1;
    }
}

Prec right_prec(Op op) {
    return prec(op) + (assoc(op) == Assoc::Left ? 1 : 0);
}

Assoc assoc(Op op) {
    switch (op) {
    case Op::Is:
    case Op::As:
    case Op::MemberAccess:
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
    case Op::Sum:
    case Op::Diff:
    case Op::ShiftLeft:
    case Op::ShiftRight:
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
    case Op::Equal:
    case Op::NotEqual:
    case Op::BwAnd:
    case Op::BwXor:
    case Op::BwOr:
    case Op::And:
    case Op::Or:
    case Op::PostInc:
    case Op::PostDec:
    case Op::Comma:
        return Assoc::Left;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Assign:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return Assoc::Right;
    default:
        assert(false);
    }
}

} // namespace ulam::ops

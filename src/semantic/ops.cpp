#include "libulam/semantic/ops.hpp"
#include <cassert>

namespace ulam::ops {

const char* str(Op op) {
#define OP(str, op) case Op::op: return str;
    switch (op) {
#include <libulam/semantic/ops.inc.hpp>
    default:
        assert(false);
    }
#undef OP
}

Prec prec(Op op) {
    switch (op) {
    case Op::FunCall:
    case Op::ArrayAccess:
    case Op::MemberAccess:
    case Op::PreInc:
    case Op::PreDec:
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
        return 1;
    default:
        return -1;
    }
}

Prec right_prec(Op op) {
    return prec(op) + (assoc(op) == Assoc::Left ? 1 : 0);
}

Assoc assoc(Op op) {
    switch (op) {
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
        return Assoc::Left;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Assign:
        return Assoc::Right;
    default:
        assert(false);
    }
}

} // namespace ulam::op

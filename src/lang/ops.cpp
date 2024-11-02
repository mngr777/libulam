#include "libulam/lang/ops.hpp"
#include <cassert>

namespace ulam::ops {

const char* str(Op op) {
#define OP(str, op) case Op::op: return str;
    switch (op) {
#include <libulam/lang/ops.inc.hpp>
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
        return 5;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
        return 4;
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
        return 3;
    case Op::Sum:
    case Op::Diff:
        return 2;
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
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
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
        return Assoc::Left;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PreInc:
    case Op::PreDec:
        return Assoc::Right;
    default:
        assert(false);
    }
}

} // namespace ulam::op

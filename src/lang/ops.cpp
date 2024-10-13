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
    case Op::MemberAccess:
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
        return Assoc::Left;
    default:
        assert(false);
    }
}

} // namespace ulam::op

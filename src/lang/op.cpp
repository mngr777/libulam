#include "libulam/lang/op.hpp"
#include <cassert>

namespace ulam::op {

const char* str(Op op) {
#define OP(str, op) case Op::op: return str;
    switch (op) {
#include <libulam/lang/op.inc.hpp>
    default:
        assert(false);
    }
#undef OP
}

Prec prec(Op op) {
    switch (op) {
    case Op::unary_minus:
    case Op::unary_plus:
        return 4;
    case Op::prod:
    case Op::quot:
    case Op::rem:
        return 3;
    case Op::sum:
    case Op::diff:
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
    case Op::prod:
    case Op::quot:
    case Op::rem:
    case Op::sum:
    case Op::diff:
        return Assoc::Left;
    default:
        assert(false);
    }
}

} // namespace ulam::op

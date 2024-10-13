#include "libulam/token.hpp"
#include <cassert>

namespace ulam::tok {

const char* type_str(tok::Type type) {
    switch (type) {
#define TOK(str, type) case type: return str;
#include "libulam/token.inc.hpp"
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}


const char* type_name(tok::Type type) {
    switch (type) {
#define TOK(str, type) case type: return #type;
#include "libulam/token.inc.hpp"
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}

Op bin_op(Type type) {
    switch (type) {
    case Dot:
        return Op::MemberAccess;
    case Plus:
        return Op::Sum;
    case Minus:
        return Op::Diff;
    case Mult:
        return Op::Prod;
    case Div:
        return Op::Quot;
    default:
        return Op::None;
    }
}

Op unary_pre_op(Type type) {
    switch (type) {
    case Plus:
        return Op::UnaryPlus;
    case Minus:
        return Op::UnaryMinus;
    case Inc:
        return Op::PreInc;
    case Dec:
        return Op::PreDec;
    default:
        return Op::None;
    }
}

Op unary_post_op(Type type) {
    switch (type) {
    case Inc:
        return Op::PostInc;
    case Dec:
        return Op::PostDec;
    default:
        return Op::None;
    }
}

} // namespace ulam::tok



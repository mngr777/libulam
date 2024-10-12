#include "libulam/token.hpp"
#include "libulam/lang/op.hpp"
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
    case Plus:
        return Op::sum;
    case Minus:
        return Op::diff;
    case Mult:
        return Op::prod;
    case Div:
        return Op::quot;
    default:
        return Op::none;
    }
}

Op unary_pre_op(Type type) {
    switch (type) {
    case Plus:
        return Op::unary_plus;
    case Minus:
        return Op::unary_minus;
    case Inc:
        return Op::pre_inc;
    case Dec:
        return Op::pre_dec;
    default:
        return Op::none;
    }
}

Op unary_post_op(Type type) {
    switch (type) {
    case Inc:
        return Op::post_inc;
    case Dec:
        return Op::post_dec;
    default:
        return Op::none;
    }
}

} // namespace ulam::tok



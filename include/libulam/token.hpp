#pragma once
#include "libulam/types.hpp"
#include "libulam/lang/op.hpp"

namespace ulam {
namespace tok {

enum Type : std::uint8_t {
#define TOK(str, type) type,
#include "token.inc.hpp"
#undef TOK
};

// NOTE: `type_str' may return nullptr
const char* type_str(Type type);
const char* type_name(Type type);

Op bin_op(Type type);
Op unary_pre_op(Type type);
Op unary_post_op(Type type);

} // namespace tok

struct Token {
    tok::Type type{tok::None};
    SrcLocId loc_id{NoSrcLocId};
    StrId str_id{NoStrId};

    const char* type_str() const { return tok::type_str(type); }
    const char* type_name() const { return tok::type_name(type); }

    bool is(tok::Type typ) const { return typ == type; }
    bool is(tok::Type typ1, tok::Type typ2) const {
        return is(typ1) || is(typ2);
    }

    Op bin_op() const { return tok::bin_op(type); }
    Op unary_pre_op() const { return tok::bin_op(type); }
    Op unary_post_op() const { return tok::bin_op(type); }
};

} // namespace ulam

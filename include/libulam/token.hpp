#pragma once
#include "libulam/types.hpp"

namespace ulam {
namespace tok {

enum Type : std::uint8_t {
#define TOK(str, type) type,
#define TOK_SEL_ALL
#include "token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
};

// NOTE: `type_str' may return nullptr
const char* type_str(const tok::Type type);
const char* type_name(const tok::Type type);

} // namespace tok

struct Token {
    bool is(tok::Type typ) const { return typ == type; }
    bool is(tok::Type typ1, tok::Type typ2) const {
        return is(typ1) || is(typ2);
    }

    const char* type_str() const { return tok::type_str(type); }
    const char* type_name() const { return tok::type_name(type); }

    tok::Type type{tok::None};
    SrcLocId loc_id{NoSrcLocId};
    StrId str_id{NoStrId};
};

} // namespace ulam

#pragma once
#include "types.hpp"

namespace ulam {
namespace tok {

enum Type {
#define TOK(str, type) type,
#define TOK_SEL_ALL
#include "token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
};

} // namespace tok

struct Token {
    tok::Type type {tok::None};
    SrcLocId loc_id {NoSrcLocId};
    StrId str_id {NoStrId};
};

} // namespace ulam


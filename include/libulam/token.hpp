#pragma once
#include "libulam/types.hpp"
#include <string>

namespace ulam {
namespace tok {

enum Type : std::uint8_t {
#define TOK(str, type) type,
#define TOK_SEL_ALL
#include "token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
};

std::string type_to_str(tok::Type type);

} // namespace tok

struct Token {
    tok::Type type {tok::None};
    SrcLocId loc_id {NoSrcLocId};
    StrId str_id {NoStrId};
};

} // namespace ulam


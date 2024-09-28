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

// NOTE: `type_str' may return nullptr
const char* type_str(const tok::Type type);
const char* type_name(const tok::Type type);

} // namespace tok

struct Token {
    tok::Type type {tok::None};
    SrcLocId loc_id {NoSrcLocId};
    StrId str_id {NoStrId};
};

} // namespace ulam


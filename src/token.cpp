#include "libulam/token.hpp"
#include <cassert>

namespace ulam::tok {

std::string type_str(tok::Type type) {
    switch (type) {
#define TOK(str, type) case tok::type: return str ? str : "";
#define TOK_SEL_ALL
#include "libulam/token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}


std::string type_name(tok::Type type) {
    switch (type) {
#define TOK(str, type) case tok::type: return #type;
#define TOK_SEL_ALL
#include "libulam/token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}

} // namespace ulam::tok


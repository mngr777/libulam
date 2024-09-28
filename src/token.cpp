#include "libulam/token.hpp"
#include <cassert>

namespace ulam::tok {

const char* type_str(const tok::Type type) {
    switch (type) {
#define TOK(str, type) case tok::type: return str;
#define TOK_SEL_ALL
#include "libulam/token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}


const char* type_name(const tok::Type type) {
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


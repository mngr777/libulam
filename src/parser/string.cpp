#include "src/parser/string.hpp"
#include "src/detail/string.hpp"
#include <cassert>
#include <libulam/diag.hpp>

namespace ulam::detail {

// TODO: support hex/oct escape sequences
String parse_str(Diag& diag, loc_id_t loc_id, const std::string_view str) {
    assert(str.size() > 0);
    std::string parsed;
    parsed.reserve(str.size());
    std::size_t cur = 0;
    const char quote = str[cur++];
    bool is_terminated = false;
    assert(quote == '\'' || quote == '"');
    while (cur < str.size()) {
        char ch = str[cur++];
        if (ch == quote) {
            is_terminated = true;
            break;
        }
        if (ch == '\\') {
            assert(cur + 1 < str.size());
            parsed += escaped(str[cur++]);
        } else {
            parsed += ch;
        }
    }
    assert(cur == str.size());
    if (!is_terminated)
        diag.emit(Diag::Error, loc_id, cur, 0, "untermitated string");
    parsed.shrink_to_fit();
    return parsed;
}

} // namespace ulam::detail

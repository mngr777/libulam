#include "src/parser/string.hpp"
#include "src/detail/string.hpp"
#include <cassert>
#include <libulam/diag.hpp>

namespace ulam::detail {

// TODO: support hex/oct escape sequences
std::string parse_str(Diag& diag, loc_id_t loc_id, const std::string_view str) {
    assert(str.size() > 0);
    std::string parsed;
    parsed.reserve(str.size());
    std::size_t cur = 0;

    const unsigned short CharMax = 255;
    const char Quote = str[cur++];
    assert(Quote == '<' || Quote == '"');
    const char Closing = (Quote == '<') ? '>' : Quote;

    bool is_terminated = false;
    while (cur < str.size()) {
        char ch = str[cur++];
        if (ch == Closing) {
            is_terminated = true;
            break;
        }
        if (ch == '\\') {
            assert(cur + 1 < str.size());
            if (detail::is_digit(str[cur])) {
                const unsigned short Base = 8;
                const unsigned DigitNum = 3;

                auto end = cur + DigitNum;
                if (end > str.size()) {
                    // not enough digits
                    diag.error(loc_id, cur, 0, "invalid char code");
                    cur = str.size();

                } else {
                    // parse char code
                    unsigned short code = 0;
                    for (; cur < end; ++cur) {
                        if (code < 0)
                            continue;
                        unsigned short dv = digit_value(str[cur]);
                        if (dv == NotDigit || dv + 1 > Base) {
                            diag.error(loc_id, cur, 0, "invalid digit");
                            code = -1;
                            continue;
                        }
                        code = code * Base + dv;
                        if (code > CharMax) {
                            diag.error(loc_id, cur, 0, "invalid char code");
                            code = -1;
                            continue;
                        }
                    }
                    parsed += (char)code;
                }

            } else {
                parsed += escaped(str[cur++]);
            }
        } else {
            parsed += ch;
        }
    }
    assert(cur == str.size());
    if (!is_terminated)
        diag.error(loc_id, cur, 0, "untermitated string");
    parsed.shrink_to_fit();
    return parsed;
}

} // namespace ulam::detail

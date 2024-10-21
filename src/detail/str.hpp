#pragma once

namespace ulam::detail {

inline bool is_whitespace(char ch) {
    switch (ch) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        return true;
    default:
        return false;
    }
}

inline bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }
inline bool is_xdigit(char ch) {
    return is_digit(ch) || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
}
inline bool is_upper(char ch) { return 'A' <= ch && ch <= 'Z'; }
inline bool is_lower(char ch) { return 'a' <= ch && ch <= 'z'; }
inline bool is_word(char ch) {
    return ch == '_' || is_upper(ch) || is_lower(ch);
}

} // namespace ulam::detail

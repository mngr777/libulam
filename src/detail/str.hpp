#pragma once
#include <cstdint>

namespace ulam::detail {

// TODO: replace comparisons with switch

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
    return ch == '_' || is_upper(ch) || is_lower(ch) || is_digit(ch);
}

constexpr std::uint8_t NotDigit = -1;

inline constexpr std::uint8_t digit_value(char digit) {
    if ('0' <= digit && digit <= '9')
        return digit - '0';
    if ('A' <= digit && digit <= 'F')
        return 10 + digit - 'A';
    if ('a' <= digit && digit <= 'f')
        return 10 + digit - 'a';
    return NotDigit;
}

} // namespace ulam::detail

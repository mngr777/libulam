#pragma once
#include <cstdint>

namespace ulam::detail {

constexpr bool is_whitespace(char ch) {
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

constexpr bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }
constexpr bool is_xdigit(char ch) {
    return is_digit(ch) || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
}
constexpr bool is_upper(char ch) { return 'A' <= ch && ch <= 'Z'; }
constexpr bool is_lower(char ch) { return 'a' <= ch && ch <= 'z'; }
constexpr bool is_word(char ch) {
    return ch == '_' || is_upper(ch) || is_lower(ch) || is_digit(ch);
}

constexpr std::uint8_t NotDigit = -1;

constexpr std::uint8_t digit_value(char digit) {
    if ('0' <= digit && digit <= '9')
        return digit - '0';
    if ('A' <= digit && digit <= 'F')
        return 10 + digit - 'A';
    if ('a' <= digit && digit <= 'f')
        return 10 + digit - 'a';
    return NotDigit;
}

constexpr char escaped(char ch) {
    switch (ch) {
    case '0':
        return '\0';
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    default:
        return ch;
    }
}

} // namespace ulam::detail

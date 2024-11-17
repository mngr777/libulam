#include "src/parser/number.hpp"
#include "src/detail/string.hpp"
#include <cassert>
#include <cstddef>
#include <libulam/diag.hpp>
#include <limits>
#include <string>

namespace ulam::detail {
namespace {

constexpr std::uint64_t MaxUnsigned = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t MaxSigned = std::numeric_limits<std::int64_t>::max();

constexpr const char* radix_to_str(std::uint8_t radix) {
    switch (radix) {
    case 2:
        return "binary";
    case 8:
        return "octal";
    case 10:
        return "decimal";
    case 16:
        return "hexadecimal";
    default:
        assert(false);
    }
}

// Max unsigned number that can be safely multiplied by radix
constexpr std::uint64_t radix_threshold(std::uint8_t radix) {
    assert(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    return MaxUnsigned / radix;
}

// Remainder of max unsigned number division by threshold value
constexpr std::uint64_t radix_threshold_rem(std::uint8_t radix) {
    return MaxUnsigned - radix_threshold(radix) * radix;
}

} // namespace

// NOTE: numeric literals cannot have a sign
// TODO: implement exp notation
Number parse_num_str(Diag& diag, loc_id_t loc_id, const std::string_view str) {
    assert(str.size() > 0);
    assert(is_digit(str[0])); // guaranteed by lexer
    Number number{};

    std::size_t cur = 0;
    std::uint64_t value = 0; // abs value

    // Radix
    if (str[cur] == '0' && str.size() > 1) {
        switch (str[cur + 1]) {
        case 'B':
        case 'b':
            number.radix = 2;
            cur += 2;
            // are there any digits after prefix?
            if (cur == str.size() || !is_digit(str[cur])) {
                diag.emit(
                    diag::Error, loc_id, str.size(),
                    "incomplete binary number");
                return number;
            }
            break;
        case 'X':
        case 'x':
            number.radix = 16;
            cur += 2;
            // are there any hex digits after prefix?
            if (cur == str.size() || !is_xdigit(str[cur])) {
                diag.emit(
                    diag::Error, loc_id, str.size(),
                    "incomplete hexadecimal number");
                return number;
            }
            break;
        default:
            // NOTE: leave single '0' decimal
            if (is_digit(str[cur + 1])) {
                number.radix = 8;
                cur += 1;
            }
        }
    }

    // Eat leading zeros
    while (cur < str.size() && str[cur] == '0')
        ++cur;

    // Value
    auto threshold = radix_threshold(number.radix);
    auto threshold_rem = radix_threshold_rem(number.radix);
    for (; cur < str.size(); ++cur) {
        std::uint8_t dv = digit_value(str[cur]);
        // is part of suffix?
        if (dv == NotDigit)
            break;
        // is valid for radix?
        if (dv + 1 > number.radix) {
            diag.emit(
                diag::Error, loc_id, cur, 1,
                std::string("invalid digit in ") + radix_to_str(number.radix) +
                    " number");
            return number;
        }
        // already overflown?
        if (number.overflow)
            continue;
        // will overflow?
        if (value > threshold || (value == threshold && dv > threshold_rem)) {
            value = MaxUnsigned;
            number.overflow = true;
            continue;
        }
        // Update value
        value = value * number.radix + dv;
    }

    // Suffix
    if (cur < str.size()) {
        if (str[cur] == 'U' || str[cur] == 'u') {
            number.is_signed = false;
            ++cur;
        }
        if (cur < str.size()) {
            diag.emit(
                diag::Error, loc_id, cur, str.size() - cur,
                "invalid number suffix");
            return number;
        }
    }

    // Overflow
    if (number.is_signed) {
        if (value > MaxSigned) {
            number.overflow = true;
            diag.emit(
                diag::Error, loc_id, str.size(),
                "number overflow");
            value = MaxSigned;
        }
        number.value = value;
    } else {
        number.uvalue = value;
    }
    return number;
}

} // namespace ulam::detail

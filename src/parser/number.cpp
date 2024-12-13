#include "src/parser/number.hpp"
#include "src/detail/string.hpp"
#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/semantic/value/types.hpp>
#include <limits>
#include <string>

namespace ulam::detail {
namespace {

constexpr Unsigned MaxSigned = std::numeric_limits<Integer>::max();
constexpr Unsigned MaxUnsigned = std::numeric_limits<Unsigned>::max();

// Max unsigned number that can be safely multiplied by radix
constexpr std::uint64_t radix_threshold(Radix radix) {
    return MaxUnsigned / radix_to_int(radix);
}

// Remainder of max unsigned number divided by threshold value
constexpr std::uint64_t radix_threshold_rem(Radix radix) {
    return MaxUnsigned - radix_threshold(radix) * radix_to_int(radix);
}

} // namespace

// NOTE: numeric literals cannot have a sign
// TODO: implement exp notation
Number parse_num_str(Diag& diag, loc_id_t loc_id, const std::string_view str) {
    assert(str.size() > 0);
    assert(is_digit(str[0])); // guaranteed by lexer

    std::size_t cur = 0;
    Unsigned value = 0; // abs value

    Radix radix = Radix::Decimal;
    bool overflow = false;
    bool is_signed = true;

    // Radix
    if (str[cur] == '0' && str.size() > 1) {
        switch (str[cur + 1]) {
        case 'B':
        case 'b':
            radix = Radix::Binary;
            cur += 2;
            // are there any digits after prefix?
            if (cur == str.size() || !is_digit(str[cur])) {
                diag.emit(
                    diag::Error, loc_id, str.size(),
                    "incomplete binary number");
                return {radix, (Integer)0};
            }
            break;
        case 'X':
        case 'x':
            radix = Radix::Hexadecimal;
            cur += 2;
            // are there any hex digits after prefix?
            if (cur == str.size() || !is_xdigit(str[cur])) {
                diag.emit(
                    diag::Error, loc_id, str.size(),
                    "incomplete hexadecimal number");
                return {radix, (Integer)0};
            }
            break;
        default:
            // NOTE: leave single '0' decimal
            if (is_digit(str[cur + 1])) {
                radix = Radix::Octal;
                cur += 1;
            }
        }
    }

    // Eat leading zeros
    while (cur < str.size() && str[cur] == '0')
        ++cur;

    // Value
    auto threshold = radix_threshold(radix);
    auto threshold_rem = radix_threshold_rem(radix);
    for (; cur < str.size(); ++cur) {
        std::uint8_t dv = digit_value(str[cur]);
        // is part of suffix?
        if (dv == NotDigit)
            break;
        // is valid for radix?
        if (dv + 1 > radix_to_int(radix)) {
            diag.emit(
                diag::Error, loc_id, cur, 1,
                std::string("invalid digit in ") + radix_to_str(radix) +
                    " number");
            return {radix, (Integer)0};
        }
        // already overflown?
        if (overflow)
            continue;
        // will overflow?
        if (value > threshold || (value == threshold && dv > threshold_rem)) {
            value = MaxUnsigned;
            overflow = true;
            continue;
        }
        // update value
        value = value * radix_to_int(radix) + dv;
    }

    // Suffix
    if (cur < str.size()) {
        if (str[cur] == 'U' || str[cur] == 'u') {
            is_signed = false;
            ++cur;
        }
        if (cur < str.size()) {
            diag.emit(
                diag::Error, loc_id, cur, str.size() - cur,
                "invalid number suffix");
        }
    }

    // Overflow?
    overflow = overflow || (is_signed && value > MaxSigned);
    if (overflow) {
        diag.emit(diag::Error, loc_id, str.size(), "number overflow");
        if (is_signed)
            value = MaxSigned;
    }

    // Done
    return is_signed ? Number{radix, (Integer)value} : Number{radix, value};
}

} // namespace ulam::detail

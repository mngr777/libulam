#pragma once
#include <cassert>
#include <cstdint>
#include <libulam/semantic/value.hpp>
#include <ostream>
#include <variant>

namespace ulam {
class Number;
}
std::ostream& operator<<(std::ostream& os, const ulam::Number& number);

namespace ulam {

enum class Radix { Binary, Octal, Decimal, Hexadecimal };

class Number {
public:
    Number(Radix radix, Integer value): _radix{radix}, _value{value} {}
    Number(Radix radix, Unsigned value): _radix{radix}, _value{value} {}

    Radix radix() const { return _radix; }

    bool is_signed() const {
        return std::holds_alternative<std::int64_t>(_value);
    }

    std::uint8_t bitsize() const;

    template <typename T> T value() const {
        assert(std::holds_alternative<T>(_value));
        return std::get<T>(_value);
    }

    void write_value(std::ostream& os) const;

private:
    Radix _radix;
    std::variant<Integer, Unsigned> _value;
};

constexpr std::uint8_t radix_to_int(Radix radix) {
    switch (radix) {
    case Radix::Binary:
        return 2;
    case Radix::Octal:
        return 8;
    case Radix::Decimal:
        return 10;
    case Radix::Hexadecimal:
        return 16;
    default:
        assert(false);
    }
}

constexpr const char* radix_to_str(Radix radix) {
    switch (radix) {
    case Radix::Binary:
        return "binary";
    case Radix::Octal:
        return "octal";
    case Radix::Decimal:
        return "decimal";
    case Radix::Hexadecimal:
        return "hexadecimal";
    default:
        assert(false);
    }
}

} // namespace ulam

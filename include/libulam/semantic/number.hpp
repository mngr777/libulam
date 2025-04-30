#pragma once
#include <cassert>
#include <cstdint>
#include <libulam/semantic/value/types.hpp>
#include <ostream>
#include <string>
#include <variant>

namespace ulam {
class Number;
}
std::ostream& operator<<(std::ostream& os, const ulam::Number& number);

namespace ulam {

enum class Radix { Binary, Octal, Decimal, Hexadecimal };

class Number {
public:
    Number(Radix radix, Integer value, bitsize_t size = 0);
    Number(Radix radix, Unsigned value, bitsize_t size = 0);
    Number();

    Radix radix() const { return _radix; }

    bool is_signed() const { return std::holds_alternative<Integer>(_value); }

    Unsigned as_unsigned() const {
        if (is_signed()) {
            assert(value<Integer>() >= 0);
            return (Unsigned)value<Integer>();
        }
        return value<Unsigned>();
    }

    std::uint8_t bitsize() const;

    template <typename T> T value() const {
        assert(std::holds_alternative<T>(_value));
        return std::get<T>(_value);
    }

    void write_value(std::ostream& os) const;

    std::string str() const;

private:
    Radix _radix;
    std::variant<Integer, Unsigned> _value;
    bitsize_t _size;
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

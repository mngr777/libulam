#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/number.hpp>

std::ostream& operator<<(std::ostream& os, const ulam::Number& number) {
    number.write_value(os);
    return os;
}

namespace ulam {

std::uint8_t Number::bitsize() const {
    return is_signed() ? detail::bitsize(value<Integer>())
                       : detail::bitsize(value<Unsigned>());
}

void Number::write_value(std::ostream& os) const {
    auto write = [&](std::ostream& os) {
        if (is_signed()) {
            os << value<Integer>();
        } else {
            os << value<Unsigned>();
        }
    };
    switch (_radix) {
    case Radix::Binary:
        write(os); // TODO
    break;
    case Radix::Octal:
        write(os << "0" << std::oct);
        break;
    case Radix::Decimal:
        write(os << std::dec);
        break;
    case Radix::Hexadecimal:
        write(os << "0x" << std::hex);
        break;
    }
}

} // namespace ulam

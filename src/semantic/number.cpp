#include <libulam/semantic/number.hpp>
#include <libulam/utils/integer.hpp>
#include <sstream>

std::ostream& operator<<(std::ostream& os, const ulam::Number& number) {
    number.write_value(os);
    return os;
}

namespace ulam {

Number::Number(Radix radix, Integer value, bitsize_t size):
    _radix{radix}, _value{value}, _size{size} {
    assert(size == 0 || size >= utils::bitsize(value));
}

Number::Number(Radix radix, Unsigned value, bitsize_t size):
    _radix{radix}, _value{value}, _size{size} {
    assert(size == 0 || size >= utils::bitsize(value));
}

Number::Number(): Number{Radix::Decimal, (Integer)0} {}

std::uint8_t Number::bitsize() const {
    if (_size != 0)
        return _size;
    return is_signed() ? utils::bitsize(value<Integer>())
                       : utils::bitsize(value<Unsigned>());
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
        if (!is_signed())
            os << "u";
        break;
    case Radix::Hexadecimal:
        write(os << "0x" << std::hex);
        break;
    }
}

std::string Number::str() const {
    std::ostringstream ss;
    write_value(ss);
    return ss.str();
}

} // namespace ulam

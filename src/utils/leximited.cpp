#include <libulam/utils/integer.hpp>
#include <libulam/utils/leximited.hpp>

// see `ULAM/src/ulam/UlamUtils.cpp`

namespace ulam::detail {
namespace {
// see `MFM/src/core/util/Util.h`
// NOTE: this version returns 1 for value = 0
Unsigned digit_num(Unsigned value, Unsigned base = 10) {
    Unsigned num = 0;
    do {
        ++num;
        value /= base;
    } while (value > 0);
    return num;
}

void write_header(std::ostream& os, Unsigned len) {
    os << len;
    if (len >= 9)
        write_header(os, digit_num(len));
}
} // namespace

void write_leximited(std::ostream& os, Integer value) {
    switch (ulam::utils::sign(value)) {
    case -1:
        os << "n";
        if (value == ulam::utils::min<Integer>()) {
            os << "10"; // "n10" is special case for min negative
        } else {
            write_leximited(os, (Unsigned)-value);
        }
        break;
    case 0:
        os << "10";
        break;
    case 1:
        write_leximited(os, (Unsigned)value);
        break;
    }
}

void write_leximited(std::ostream& os, Unsigned value) {
    write_header(os, digit_num(value));
    os << value;
}

void write_leximited(std::ostream& os, std::string_view value) {
    write_header(os, value.size());
    os << value;
}

void write_leximited(std::ostream& os, char value) {
    write_header(os, 1);
    os << value;
}

void write_leximited(std::ostream& os, bool value) {
    write_leximited(os, (Unsigned)(value ? 1 : 0));
}

} // namespace ulam::detail

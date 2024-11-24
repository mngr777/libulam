#include <libulam/semantic/number.hpp>

namespace ulam {

std::uint8_t Number::bitsize() const {
    Unsigned abs;
    if (is_signed()) {
        Integer val = value<Integer>();
        abs = (val < 0) ? -val : val;
    } else {
        abs = value<Unsigned>();
    }
    std::uint8_t size = 0;
    do {
        ++size;
        abs >>= 1;
    } while (abs > 0);
    return size;
}

} // namespace ulam

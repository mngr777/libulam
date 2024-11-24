#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/number.hpp>

namespace ulam {

std::uint8_t Number::bitsize() const {
    return is_signed() ? detail::bitsize(value<Integer>())
                       : detail::bitsize(value<Unsigned>());
}

} // namespace ulam

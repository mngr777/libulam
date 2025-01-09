#pragma once
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam::cls {

using data_off_t = BitVector::size_t;
constexpr data_off_t NoDataOff = -1;

} // namespace ulam::cls

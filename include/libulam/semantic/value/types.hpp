#pragma once
#include <cstdint>
#include <string>

#define ULAM_MAX_INT_SIZE (sizeof(Integer) * 8)

namespace ulam {

using bitsize_t = std::uint16_t;
constexpr bitsize_t NoBitsize = 0;

using Integer = std::int32_t;
using Unsigned = std::uint32_t;
using Bool = bool; // TODO: remove
using String = std::string; // TODO: struct { str_id_t id; }

using Datum = Unsigned; // binary representation
static_assert(sizeof(Datum) == sizeof(Integer));
static_assert(sizeof(Datum) == sizeof(Unsigned));

} // namespace ulam

#pragma once
#include <cstdint>
#include <string>

#define ULAM_MAX_INT_SIZE (sizeof(Integer) * 8)

namespace ulam {

using Integer = std::int64_t;
using Unsigned = std::uint64_t;
using Bool = bool;
using String = std::string;

} // namespace ulam

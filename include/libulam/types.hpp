#pragma once
#include <cstdint>

namespace ulam {

using SrcId = std::uint32_t;
constexpr std::uint32_t NoSrcId = -1;

using SrcLocId = std::uint32_t;
constexpr SrcLocId NoSrcLocId = -1;

using LineNum = std::uint32_t;
using CharNum = std::uint32_t;

using StrId = std::uint32_t;
constexpr StrId NoStrId = -1;

} // namespace ulam


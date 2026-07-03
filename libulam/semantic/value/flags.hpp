#pragma once
#include <cstdint>

namespace ulam::value {

using flags_t = std::uint8_t;
constexpr flags_t NoFlags = 0;
constexpr flags_t IsConsteval = 1;
constexpr flags_t IsXvalue = 1 << 1;
constexpr flags_t LValueFlags = IsConsteval | IsXvalue;
constexpr flags_t RValueFlags = IsConsteval;

} // namespace value

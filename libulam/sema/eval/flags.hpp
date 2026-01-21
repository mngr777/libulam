#pragma once
#include <cstdint>

namespace ulam::sema {

using eval_flags_t = std::uint16_t;

namespace evl {
static constexpr eval_flags_t NoFlags = 0;
static constexpr eval_flags_t NoExec = 1;
static constexpr eval_flags_t Consteval = 1 << 1;
static constexpr eval_flags_t NoDerefCast = 1 << 2;
static constexpr eval_flags_t Last = 1 << 9;
} // namespace evl

} // namespace ulam::sema

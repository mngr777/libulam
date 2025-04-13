#pragma once
#include <cstdint>

namespace ulam::sema {

using eval_flags_t = std::uint16_t;

namespace evl {
static constexpr eval_flags_t NoFlags = 0;
static constexpr eval_flags_t NoExec = 1;
}

} // namespace ulam::sema

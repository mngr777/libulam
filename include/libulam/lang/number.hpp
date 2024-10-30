#pragma once
#include <cstdint>

namespace ulam {

struct Number {
    std::uint8_t radix{10};
    bool is_signed{true};
    union {
        std::int64_t value{0};
        std::uint64_t uvalue;
    };
    bool overflow = false;
};

} // namespace ulam

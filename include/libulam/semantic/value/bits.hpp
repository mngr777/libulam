#pragma once
#include <cstdint>
#include <libulam/memory/buf.hpp>
#include <variant>

namespace ulam {

class Bits {
public:
    Bits(std::uint64_t data): _value{data} {}

    // TODO: buffer

private:
    std::variant<std::uint64_t, mem::Buf> _value;
};

} // namespace ulam

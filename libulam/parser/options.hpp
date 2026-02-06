#pragma once
#include <libulam/types.hpp>

namespace ulam {

struct ParserOptions {
    bool allow_assign_in_ternary{false};
};

const ParserOptions DefaultParserOptions{};

} // namespace ulam

#pragma once
#include <libulam/types.hpp>

namespace ulam {

struct ParserOptions {
    bool allow_assign_in_ternary{false};
    PathList global_include_paths;
    PathList include_paths;
};

const ParserOptions DefaultParserOptions{};

} // namespace ulam

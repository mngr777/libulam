#pragma once

namespace ulam {

struct ParserOptions {
    bool allow_assign_in_ternary{false};
};

const ParserOptions DefaultParserOptions{};

} // namespace ulam

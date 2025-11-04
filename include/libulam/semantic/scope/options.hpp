#pragma once

namespace ulam {

struct ScopeOptions {
    bool allow_access_before_def{true}; // TODO: false by default
};

constexpr ScopeOptions DefaultScopeOptions{};

} // namespace ulam

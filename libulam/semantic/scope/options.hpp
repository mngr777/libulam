#pragma once

namespace ulam {

struct ScopeOptions {
    bool allow_access_before_def{true};
    bool prefer_params_in_param_resolution{true};
};

constexpr ScopeOptions DefaultScopeOptions{};

} // namespace ulam

#pragma once
#include <cstdint>

namespace ulam {

using ScopeFlags = std::uint16_t;

namespace scp {
static constexpr ScopeFlags NoFlags = 0;
static constexpr ScopeFlags Persistent = 1;
static constexpr ScopeFlags Program = 1 << 1;
static constexpr ScopeFlags ModuleEnv = 1 << 2;
static constexpr ScopeFlags Module = 1 << 3;
static constexpr ScopeFlags Class = 1 << 4;
static constexpr ScopeFlags ClassTpl = 1 << 5;
static constexpr ScopeFlags Fun = 1 << 6;
static constexpr ScopeFlags Break = 1 << 7;
static constexpr ScopeFlags Continue = 1 << 8;
} // namespace scp

} // namespace ulam

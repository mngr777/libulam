#pragma once
#include <cstdint>

namespace ulam {

using ScopeFlags = std::uint16_t;

namespace scp {
static constexpr ScopeFlags NoFlags = 0;
static constexpr ScopeFlags Persistent = 1;
static constexpr ScopeFlags Program = 1 << 1;   // 2
static constexpr ScopeFlags ModuleEnv = 1 << 2; // 4
static constexpr ScopeFlags Module = 1 << 3;    // 8
static constexpr ScopeFlags Class = 1 << 4;     // 16
static constexpr ScopeFlags ClassTpl = 1 << 5;  // 32
static constexpr ScopeFlags Fun = 1 << 6;       // 64
static constexpr ScopeFlags Break = 1 << 7;     // 128
static constexpr ScopeFlags Continue = 1 << 8;  // 256
} // namespace scp

} // namespace ulam

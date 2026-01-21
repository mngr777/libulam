#pragma once
#include <cstdint>

namespace ulam {

using scope_flags_t = std::uint16_t;

namespace scp {
static constexpr scope_flags_t NoFlags = 0;
static constexpr scope_flags_t Persistent = 1;
static constexpr scope_flags_t Program = 1 << 1;   // 2
static constexpr scope_flags_t ModuleEnv = 1 << 2; // 4
static constexpr scope_flags_t Module = 1 << 3;    // 8
static constexpr scope_flags_t Class = 1 << 4;     // 16
static constexpr scope_flags_t ClassTpl = 1 << 5;  // 32
static constexpr scope_flags_t Params = 1 << 6;    // 64
static constexpr scope_flags_t Self = 1 << 7;      // 128
static constexpr scope_flags_t AsCond = 1 << 8;    // 256
static constexpr scope_flags_t Fun = 1 << 9;       // 512
static constexpr scope_flags_t Break = 1 << 10;    // 1024
static constexpr scope_flags_t Continue = 1 << 11; // 2048
static constexpr scope_flags_t Last = 1 << 11;
// combined
static constexpr scope_flags_t BreakAndContinue = Break | Continue;
} // namespace scp

} // namespace ulam

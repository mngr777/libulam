#pragma once
#include <libulam/sema/expr_res.hpp>

namespace exp {

using flags_t = ulam::sema::ExprRes::flags_t;
static constexpr flags_t NoFlags = 0;
static constexpr flags_t ImplCast = 1;
static constexpr flags_t ExplCast = 1 << 1;
static constexpr flags_t OmitCast = 1 << 2;
static constexpr flags_t Self = 1 << 3;
static constexpr flags_t SelfMemberAccess = 1 << 4;
static constexpr flags_t MemberAccess = 1 << 5;
static constexpr flags_t NumLit = 1 << 6;

} // namespace exp

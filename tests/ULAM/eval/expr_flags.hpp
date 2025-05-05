#pragma once
#include <libulam/sema/expr_res.hpp>

namespace exp {

using flags_t = ulam::sema::ExprRes::flags_t;
static constexpr flags_t NoFlags = ulam::sema::ExprRes::NoFlags;
static constexpr flags_t Self = ulam::sema::ExprRes::Self;
static constexpr flags_t First = ulam::sema::ExprRes::Last;
static constexpr flags_t ImplCast = First;
static constexpr flags_t ExplCast = First << 1;
static constexpr flags_t RefCastInternal = First << 2;
static constexpr flags_t OmitCastInternal = First << 3;
static constexpr flags_t SelfMemberAccess = First << 4;
static constexpr flags_t MemberAccess = First << 5;
static constexpr flags_t NumLit = First << 6;
static constexpr flags_t NoConstFold = First << 7;

} // namespace exp

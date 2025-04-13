#pragma once
#include <libulam/sema/expr_res.hpp>

namespace exp {
static constexpr ulam::sema::ExprRes::flags_t ImplCast = 1;
static constexpr ulam::sema::ExprRes::flags_t ExplCast = 1 << 1;
static constexpr ulam::sema::ExprRes::flags_t Self = 1 << 2;
static constexpr ulam::sema::ExprRes::flags_t ExtMemberAccess = 1 << 3;
}

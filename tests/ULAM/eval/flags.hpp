#pragma once
#include <libulam/sema/eval/flags.hpp>

namespace eval {
using eval_flags_t = ulam::sema::eval_flags_t;
static constexpr eval_flags_t First = ulam::sema::eval::Last;
static constexpr eval_flags_t NoCodegen = First;
static constexpr eval_flags_t NoConstevalCast = First << 1;
static constexpr eval_flags_t InExpr = First << 2;
static constexpr eval_flags_t NoConstFold = First << 3;
} // namespace eval

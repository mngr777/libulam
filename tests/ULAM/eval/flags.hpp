#pragma once
#include <libulam/sema/eval/flags.hpp>

namespace evl {
static constexpr ulam::sema::eval_flags_t First = ulam::sema::evl::Last;
static constexpr ulam::sema::eval_flags_t NoCodegen = First;
static constexpr ulam::sema::eval_flags_t NoConstevalCast = First << 1;
}

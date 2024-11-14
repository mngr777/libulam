#pragma once
#include <cstdint>

namespace ulam {

enum class Op {
#define OP(str, op) op,
#include <libulam/semantic/ops.inc.hpp>
#undef OP
};

namespace ops {

using Prec = std::int8_t;

enum class Assoc { Left, Right };

const char* str(Op op);

Prec prec(Op op);
Prec right_prec(Op op);
Assoc assoc(Op op);

} // namespace op
} // namespace ulam

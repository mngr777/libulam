#pragma once
#include <cstdint>

namespace ulam {

enum class Op {
#define OP(str, op) op,
#include <libulam/lang/op.inc.hpp>
#undef OP
};

namespace op {

using Prec = std::int8_t;

enum class Assoc { Left, Right };

const char* str(Op op);

Prec prec(Op op);
Prec right_prec(Op op);
Assoc assoc(Op op);

} // namespace op
} // namespace ulam

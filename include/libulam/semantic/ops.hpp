#pragma once
#include <cstdint>
#include <string_view>

namespace ulam {

enum class Op {
#define OP(str, op) op,
#include <libulam/semantic/ops.inc.hpp>
#undef OP
};

namespace ops {

enum class Kind {
    Assign,
    Equality,
    Comparison,
    Numeric,
    Logical,
    Bitwise,
    Objective
};

using Prec = std::int8_t;

enum class Assoc { Left, Right };

const char* str(Op op);

Op fun_name_op(const std::string_view name);

bool is_overloadable(Op op);

Kind kind(Op op);

Op negation(Op os);

bool is_numeric(Op op);
bool is_logical(Op op);
bool is_bitwise(Op op);
bool is_assign(Op op);
Op non_assign(Op op);

bool is_unary_op(Op op);
bool is_unary_pre_op(Op op);
bool is_unary_post_op(Op op);
bool is_inc_dec(Op op);

Prec prec(Op op);
Prec right_prec(Op op);
Assoc assoc(Op op);

} // namespace ops
} // namespace ulam

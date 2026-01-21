#pragma once

namespace ulam {

enum class TypeOp {
#define OP(str, op) op,
#include <libulam/semantic/type_ops.inc.hpp>
#undef OP
};

namespace ops {

const char* str(TypeOp op);

}

} // namespace ulam

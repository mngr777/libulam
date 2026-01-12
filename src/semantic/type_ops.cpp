#include "src/utils/unreachable.hpp"
#include <cassert>
#include <libulam/semantic/type_ops.hpp>

namespace ulam::ops {

const char* str(TypeOp op) {
#define OP(str, op)                                                            \
    case TypeOp::op:                                                           \
        return str;
    switch (op) {
#include <libulam/semantic/type_ops.inc.hpp>
    default:
        utils::unreachable();
    }
#undef OP
}

} // namespace ulam::ops

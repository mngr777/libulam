#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

class Type;

struct TypeError {
public:
    enum Status { Ok, ImplCastRequired, ExplCastRequired, Incompatible };

    TypeError(Status status);
    TypeError(Status status, BuiltinTypeId cast_bi_type_id);
    TypeError(): TypeError{Ok} {}

    Status status{Ok};
    BuiltinTypeId cast_bi_type_id{NoBuiltinTypeId};
};

using TypeErrorPair = std::pair<TypeError, TypeError>;

TypeError unary_op_type_check(Op op, Ref<const Type> type);
TypeErrorPair
binary_op_type_check(Op op, Ref<const Type> left, Ref<const Type> right);

} // namespace ulam

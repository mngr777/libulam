#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

class Type;
class TypedValue;

struct TypeError {
public:
    enum Status { Ok, ImplCastRequired, ExplCastRequired, Incompatible };

    TypeError(Status status);
    TypeError(Status status, Ref<Type> cast_type);
    TypeError(Status status, BuiltinTypeId cast_bi_type_id);
    TypeError(): TypeError{Ok} {}

    Status status{Ok};
    Ref<Type> cast_type{};
    BuiltinTypeId cast_bi_type_id{NoBuiltinTypeId};
};

using TypeErrorPair = std::pair<TypeError, TypeError>;

TypeError unary_op_type_check(Op op, Ref<Type> type);
TypeErrorPair
binary_op_type_check(Op op, Ref<Type> l_type, const TypedValue& r_tv);

} // namespace ulam

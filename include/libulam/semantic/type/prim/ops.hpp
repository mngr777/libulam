#pragma once
#include <cassert>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <list>
#include <utility>

namespace ulam {

struct PrimTypeError {
public:
    enum Status { Ok, ImplCastRequired, ExplCastRequired, Incompatible };

    bool ok() const { return status == Ok; }
    bool requires_impl_cast() const { return status == ImplCastRequired; }
    bool requires_expl_cast() const { return status == ExplCastRequired; };
    bool incompatible() const { return status == Incompatible; }

    Status status{Ok};
    BuiltinTypeId suggested_type{VoidId};
};

using PrimTypeErrorPair = std::pair<PrimTypeError, PrimTypeError>;
using PrimTypeErrorList = std::list<PrimTypeError>;

PrimTypeErrorPair prim_binary_op_type_check(
    Op op, Ref<PrimType> left_type, Ref<PrimType> right_type);

} // namespace ulam

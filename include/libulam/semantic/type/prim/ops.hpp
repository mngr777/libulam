#pragma once
#include <array>
#include <cassert>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <utility>

// TODO: this needs refactoring

namespace ulam {

struct PrimOpTypeError {
    enum Error { Ok = 0, CastSuggested, Incompatible };

    bool ok() const { return error == Ok; }

    BuiltinTypeId suggested_type_id{VoidId};
    Error error{Ok};
};

class PrimOpResBase {
public:
    template <typename... Args>
    PrimOpResBase(Args&&... args): _res{std::forward(args)...} {}

    bool ok() const { return _res.ok(); }

    ExprRes& res() { return _res; }
    const ExprRes& res() const { return _res; }

    ExprRes move_res();

private:
    ExprRes _res;
};

class PrimBinaryOpRes : public PrimOpResBase {
public:
    PrimBinaryOpRes(PrimOpTypeError left_error, PrimOpTypeError right_error);

    template <typename... Args>
    PrimBinaryOpRes(Args&&... args): PrimOpResBase{std::forward(args)...} {}

    std::array<PrimOpTypeError, 2> errors;
};

PrimBinaryOpRes binary_op(
    Op op,
    Ref<PrimType> left_type,
    const Value& left_val,
    Ref<PrimType> right_type,
    const Value& right_val);

} // namespace ulam

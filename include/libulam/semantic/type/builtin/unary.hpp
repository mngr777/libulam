#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Value;

class UnaryType : public _PrimType<UnaryId, 1, ULAM_MAX_INT_SIZE, 32> {
public:
    UnaryType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        _PrimType{builtins, id_gen, tpl, bitsize} {}

    RValue construct() override { return Unsigned{}; }
};

using UnaryTypeTpl = _PrimTypeTpl<UnaryType>;

} // namespace ulam

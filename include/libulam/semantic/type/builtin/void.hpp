#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Value;

class VoidType : public PrimType {
public:
    VoidType(TypeIdGen& id_gen): PrimType{id_gen} {}

    bitsize_t bitsize() const override { assert(false); }
};

} // namespace ulam

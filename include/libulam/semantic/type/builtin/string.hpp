#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class StringType : public PrimType {
public:
    StringType(TypeIdGen& id_gen): PrimType{id_gen} {}
};

} // namespace ulam

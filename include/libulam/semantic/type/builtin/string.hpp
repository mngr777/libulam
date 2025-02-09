#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class StringType : public PrimType {
public:
    StringType(Builtins& builtins, TypeIdGen* id_gen):
        PrimType{builtins, id_gen} {}

    bitsize_t bitsize() const override { return 8; /* TMP placeholder */ }

    // RValue construct() override { return ...; }

    BuiltinTypeId builtin_type_id() const override { return StringId; }
};

} // namespace ulam

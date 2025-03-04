#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <string>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Value;

class VoidType : public PrimType {
public:
    VoidType(Builtins& builtins, TypeIdGen& id_gen):
        PrimType{builtins, &id_gen} {}

    std::string name() const override { return "Void"; }

    bitsize_t bitsize() const override { return 0; }

    BuiltinTypeId bi_type_id() const override { return VoidId; }

    Ref<ArrayType> array_type(array_size_t size) override { assert(false); }
    Ref<RefType> ref_type() override { assert(false); }
};

} // namespace ulam

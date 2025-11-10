#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Value;

class VoidType : public PrimType {
public:
    VoidType(Builtins& builtins, TypeIdGen& id_gen):
        PrimType{builtins, &id_gen} {}

    const std::string_view name() const override { return "Void"; }

    bool is_constructible() const override { return false; }

    bitsize_t bitsize() const override { return 0; }

    BuiltinTypeId bi_type_id() const override { return VoidId; }

    Ref<ArrayType> array_type(array_size_t size) override { assert(false); }
    Ref<RefType> ref_type() override { assert(false); }
};

} // namespace ulam

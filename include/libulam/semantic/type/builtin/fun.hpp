#pragma once
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

class FunType : public Type {
public:
    FunType(Builtins& builtins, TypeIdGen& id_gen): Type{builtins, &id_gen} {}

    const std::string_view name() const override { return "Fun"; }

    bool is_constructible() const override { return false; }

    bitsize_t bitsize() const override { assert(false); }

    BuiltinTypeId bi_type_id() const override { return FunId; }

    Ref<ArrayType> array_type(array_size_t size) override { assert(false); }
    Ref<RefType> ref_type() override { assert(false); }
};

} // namespace ulam

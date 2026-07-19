#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class FunType : public Type {
    ULAM_TYPE
public:
    FunType(Builtins& builtins, TypeIdGen& id_gen): Type{builtins, &id_gen} {}

    const std::string_view name() const override { return "Fun"; }

    bitsize_t bitsize() const override;

    bool is_constructible() const override { return false; }

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    BuiltinTypeId bi_type_id() const override { return FunId; }

    Value cast_to(Ref<Type> type, Value&& val) override;

    Ref<ArrayType> array_type(array_size_t size) override;
    Ref<RefType> ref_type() override;
};

} // namespace ulam

#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <string>

namespace ulam {

class Builtins;

class AtomType : public Type {
public:
    AtomType(Builtins& builtins, TypeIdGen& id_gen): Type{builtins, &id_gen} {}

    std::string name() const override { return "Atom"; }

    bitsize_t bitsize() const override { return ULAM_ATOM_SIZE; }

    bool is_constructible() const override { return true; }
    RValue construct() override;
    RValue construct(Bits&& bits);

    BuiltinTypeId bi_type_id() const override { return AtomId; }

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;
};

} // namespace ulam

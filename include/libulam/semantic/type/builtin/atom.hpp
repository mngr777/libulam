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
    RValue construct() const override;
    RValue construct(Bits&& bits) const;

    BuiltinTypeId bi_type_id() const override { return AtomId; }

    RValue load(const BitsView data, bitsize_t off) const override;
    void store(BitsView data, bitsize_t off, const RValue& rval)
        const override;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;

    RValue cast_to(Ref<const Type> type, RValue&& rval) override;

};

} // namespace ulam

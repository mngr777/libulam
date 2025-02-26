#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

class Builtins;

class AtomType : public Type {
public:
    AtomType(Builtins& builtins, TypeIdGen& id_gen): Type{builtins, &id_gen} {}

    bitsize_t bitsize() const override { return ULAM_ATOM_SIZE; }

    BuiltinTypeId bi_type_id() const override { return AtomId; }

    RValue load(const BitVectorView data, BitVector::size_t off) const override;
    void store(BitVectorView data, BitVector::size_t off, const RValue& rval)
        const override;

    RValue construct() const override;
};

} // namespace ulam

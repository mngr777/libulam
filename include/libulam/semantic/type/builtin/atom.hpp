#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

class Builtins;

class AtomType : public Type {
public:
    AtomType(Builtins& builtins, TypeIdGen& id_gen):
        Type{&id_gen}, _builtins{builtins} {}

    bitsize_t bitsize() const override { return ULAM_ATOM_SIZE; }

    BuiltinTypeId bi_type_id() const override { return AtomId; }

private:
    Builtins& _builtins;
};

} // namespace ulam

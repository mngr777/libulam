#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/element.hpp>
#include <string>

namespace ulam {

class Builtins;

class AtomType : public Type {
public:
    using Type::is_castable_to;

    AtomType(Builtins& builtins, TypeIdGen& id_gen, ElementRegistry& elements);

    std::string name() const override { return "Atom"; }

    bitsize_t bitsize() const override { return ULAM_ATOM_SIZE; }

    bool is_constructible() const override { return true; }
    RValue construct() override;
    RValue construct(Bits&& bits);

    BuiltinTypeId bi_type_id() const override { return AtomId; }

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    Ref<Type> data_type(const BitsView data, bitsize_t off);

    bool is_castable_to(
        Ref<const Type> type,
        const Value& val,
        bool expl = true) const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

private:
    elt_id_t read_element_id(const BitsView data, bitsize_t off);

    ElementRegistry& _elements;
};

} // namespace ulam

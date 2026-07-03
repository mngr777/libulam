#include "libulam/semantic/value/flags.hpp"
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

AtomType::AtomType(
    Builtins& builtins, TypeIdGen& id_gen, ElementRegistry& elements):
    Type{builtins, &id_gen}, _elements{elements} {}

RValue AtomType::load(const BitsView data, bitsize_t off) {
    return construct(data.view(off, bitsize()).copy());
}

void AtomType::store(BitsView data, bitsize_t off, const RValue& rval) {
    ulam_assert(rval.is<DataPtr>());
    data.write(off, rval.get<DataPtr>()->bits().view());
}

Ref<Type> AtomType::data_type(const BitsView data, bitsize_t off) {
    ulam_assert(data.len() - off >= ULAM_ATOM_SIZE);
    Ref<Type> type{this};
    auto elt_id = read_element_id(data, off);
    auto elt_type = _elements.get(elt_id);
    if (elt_type)
        return elt_type;
    return type;
}

RValue AtomType::construct_default(value::flags_t rval_flags) {
    auto data = make_s<Data>(this);
    data->bits().write(AtomEltIdOff, AtomEltIdSize, NoEltId);
    return RValue::make(make_s<Data>(this), rval_flags);
}

RValue AtomType::construct(Bits&& bits, value::flags_t rval_flags) {
    ulam_assert(bits.len() == bitsize());
    auto type = data_type(bits.view(), 0);
    auto data = make_s<Data>(type, std::move(bits));
    return RValue::make(data, rval_flags);
}

bool AtomType::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    if (is_same(type))
        return true;

    if (!type->is_class())
        return false;

    if (val.has_rvalue()) {
        auto dyn_type = val.dyn_obj_type();
        if (dyn_type->is_class())
            return dyn_type->as_class()->is_castable_to(type, val, expl);
        return type->as_class()->is_element() && expl;
    }
    // unknown object value, assuming element, TODO: check if `type` as element
    // descendants
    return true;
}

bool AtomType::is_refable_as(
    Ref<const Type> type, const Value& val, bool expl) const {
    return is_castable_to(type, val, expl);
}

conv_cost_t AtomType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (type->is(AtomId))
        return 0;
    if (is_impl_castable_to(type))
        return AtomToElementConvCost;
    if (allow_cast && is_expl_castable_to(type))
        return CastCost;
    return MaxConvCost;
}

Value AtomType::cast_to(Ref<Type> type, Value&& val) {
    if (val.has_rvalue()) {
        auto dyn_type = val.dyn_obj_type();
        if (!is_same(dyn_type))
            return dyn_type->cast_to(type, std::move(val));
    }

    ulam_assert(is_expl_castable_to(type));
    ulam_assert(type->is_class() && type->as_class()->is_element());
    if (!val.has_rvalue())
        return std::move(val);

    auto rval = val.move_rvalue();
    ulam_assert(rval.is<DataPtr>());
    auto& bits = rval.get<DataPtr>()->bits();
    auto elt_rval = type->as_class()->construct_default();
    elt_rval.get<DataPtr>()->bits().write(0, bits.view());
    return Value{std::move(elt_rval)};
}

elt_id_t AtomType::read_element_id(const BitsView data, bitsize_t off) {
    return data.read(off + AtomEltIdOff, AtomEltIdSize);
}

} // namespace ulam

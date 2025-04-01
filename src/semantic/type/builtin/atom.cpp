#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

RValue AtomType::load(const BitsView data, bitsize_t off) {
    return construct(data.view(off, bitsize()).copy());
}

void AtomType::store(BitsView data, bitsize_t off, const RValue& rval) {
    assert(rval.is<DataPtr>());
    data.write(off, rval.get<DataPtr>()->bits().view());
}

RValue AtomType::construct() {
    return RValue{make_s<Data>(this)};
}

RValue AtomType::construct(Bits&& bits) {
    return RValue{make_s<Data>(this, std::move(bits))};
}

bool AtomType::is_castable_to(Ref<const Type> type, bool expl) const {
    if (type->is_class())
        return type->as_class()->is_element() && expl; // only to element?
    return false;
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
    assert(is_expl_castable_to(type));
    assert(type->is_class() && type->as_class()->is_element());
    if (val.empty())
        return std::move(val);

    auto rval = val.move_rvalue();
    assert(rval.is<DataPtr>());
    auto& bits = rval.get<DataPtr>()->bits();
    auto elt_rval = type->as_class()->construct();
    elt_rval.get<DataPtr>()->bits().write(0, bits.view());
    return Value{std::move(elt_rval)};
}

} // namespace ulam

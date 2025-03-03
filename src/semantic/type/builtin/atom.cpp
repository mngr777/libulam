#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

RValue AtomType::load(const BitsView data, bitsize_t off) const {
    assert(false);
}

void AtomType::store(BitsView data, bitsize_t off, const RValue& rval) const {
    assert(false);
}

RValue AtomType::construct() const {
    return RValue{make_s<Data>(const_cast<AtomType*>(this))};
}

RValue AtomType::construct(Bits&& bits) const {
    return RValue{make_s<Data>(const_cast<AtomType*>(this), std::move(bits))};
}

bool AtomType::is_castable_to(Ref<const Type> type, bool expl) const {
    auto canon = type->canon();
    if (canon->is_class())
        return canon->as_class()->is_element() && expl;
    return false;
}

RValue AtomType::cast_to(Ref<const Type> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(type->is_class() && type->as_class()->is_element());
    if (rval.empty())
        return std::move(rval);

    assert(rval.is<DataPtr>());
    auto& bits = rval.get<DataPtr>()->bits();
    auto elt_rval = type->as_class()->construct();
    elt_rval.get<DataPtr>()->bits().write(0, bits.view());
    return elt_rval;
}

} // namespace ulam

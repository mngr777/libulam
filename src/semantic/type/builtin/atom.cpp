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
    auto canon = type->canon();
    if (canon->is_class()) {
        if (rval.empty())
            return std::move(rval);
        auto val = rval.get<DataPtr>();
        // val->cast(this); // TODO
        return RValue{val};
    }
    assert(false);
}

} // namespace ulam

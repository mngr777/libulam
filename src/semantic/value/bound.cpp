#include <cassert>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value/bound.hpp>

namespace ulam {

// BoundProp

BoundProp BoundProp::mem_obj_bound_prop(Ref<Prop> prop) {
    return {main(), mem_obj_view(), prop};
}

BoundFunSet BoundProp::mem_obj_bound_fset(Ref<FunSet> fset) {
    return {main(), mem_obj_view(), fset};
}

ObjectView BoundProp::mem_obj_view() { return mem()->obj_view(obj_view()); }

BitVectorView BoundProp::bits_view() { return mem()->bits_view(obj_view()); }

void BoundProp::load(RValue& rval) const { rval = mem()->load(obj_view()); }

void BoundProp::store(const RValue& rval) { mem()->store(obj_view(), rval); }

} // namespace ulam

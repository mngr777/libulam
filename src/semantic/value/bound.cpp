#include <cassert>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/type/class/prop.hpp>

namespace ulam {

// BoundProp

void BoundProp::load(RValue& rval) const {
    rval = mem()->load(obj_view());
}

void BoundProp::store(const RValue& rval) { mem()->store(obj_view(), rval); }

} // namespace ulam

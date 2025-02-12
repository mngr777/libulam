#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/type/class/prop.hpp>

namespace ulam {

void BoundProp::load(RValue& rval) {
    rval = mem()->load(obj());
}

void BoundProp::store(const RValue& rval) {
    mem()->store(obj(), rval);
}

} // namespace ulam

#include <cassert>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bit_vector.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

RValue Prop::load(SPtr<const Object> obj) const {
    assert(has_type());
    assert(obj);
    return type()->load(obj->bits().view(), data_off());
}

void Prop::store(SPtr<Object> obj, const RValue& rval) {
    assert(has_type());
    assert(has_data_off());
    assert(obj);
    type()->store(obj->bits().view(), data_off(), rval);
}

} // namespace ulam

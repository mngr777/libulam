#include <cassert>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bit_vector.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

RValue Prop::load(ObjectView obj_view) const {
    assert(has_type());
    return type()->load(obj_view.bits(), data_off());
}

void Prop::store(ObjectView obj_view, const RValue& rval) {
    assert(has_type());
    assert(has_data_off());
    type()->store(obj_view.bits(), data_off(), rval);
}

} // namespace ulam

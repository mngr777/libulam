#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

ObjectView Prop::obj_view(ObjectView obj_view) {
    assert(type()->canon()->is_class());
    auto cls = type()->canon()->as_class();
    auto bits = bits_view(obj_view);
    assert(bits.len() == cls->bitsize());
    return ObjectView{cls, bits};
}

BitsView Prop::bits_view(ObjectView obj_view) {
    auto bits = obj_view.bits();
    assert(bits && bits.len() >= data_off() + type()->bitsize());
    return bits.view(data_off(), type()->bitsize());
}

RValue Prop::load(ObjectView obj_view) const {
    return type()->canon()->load(obj_view.bits(), data_off());
}

void Prop::store(ObjectView obj_view, const RValue& rval) {
    type()->canon()->store(obj_view.bits(), data_off(), rval);
}

cls::data_off_t Prop::data_off() const {
    assert(has_data_off());
    return _data_off;
}

void Prop::set_data_off(cls::data_off_t off) {
    assert(!has_data_off());
    _data_off = off;
}

} // namespace ulam

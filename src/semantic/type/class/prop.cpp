#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>

namespace ulam {

RValue Prop::load(DataPtr obj) const {
    auto off = data_off_in(obj);
    return type()->canon()->load(obj->bits(), off);
}

void Prop::store(DataPtr obj, RValue&& rval) const {
    auto off = data_off_in(obj);
    type()->canon()->store(obj->bits(), off, std::move(rval));
}

bitsize_t Prop::data_off_in(Ref<Class> derived) const {
    auto off = data_off();
    if (derived != cls())
        off += derived->base_off(cls());
    return off;
}

bitsize_t Prop::data_off_in(DataPtr obj) const {
    assert(obj->is_class());
    auto obj_cls = obj->type()->canon()->as_class();
    assert(obj_cls == cls() || obj_cls->is_base_of(cls()));
    return data_off_in(obj_cls);
}

bitsize_t Prop::data_off() const {
    assert(has_data_off());
    return _data_off;
}

void Prop::set_data_off(bitsize_t off) {
    assert(!has_data_off());
    _data_off = off;
}

} // namespace ulam

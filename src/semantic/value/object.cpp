#include "libulam/semantic/value/bit_storage.hpp"
#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

// Object

Object::Object(Ref<Class> cls): _BitStorage{cls->bitsize()}, _cls{cls} {}

Object::Object(Ref<Class> cls, BitVector&& bits_):
    _BitStorage{std::move(bits_)}, _cls{cls} {
    assert(cls);
    assert(bits().len() == cls->bitsize());
}

Object::~Object() {}

SPtr<Object> Object::copy() const {
    return make_s<Object>(_cls, bits().copy());
}

ObjectView Object::view() { return {cls(), bits().view()}; }

const ObjectView Object::view() const {
    return const_cast<Object*>(this)->view();
}

// ObjectView

ObjectView::ObjectView(Ref<Class> cls, BitVectorView bits_):
    _BitStorageView{bits_}, _cls{cls} {
    assert(bits_.len() == cls->bitsize());
}

ObjectView::operator bool() const {
    assert((bool)_cls == (bool)bits());
    return _cls;
}

} // namespace ulam

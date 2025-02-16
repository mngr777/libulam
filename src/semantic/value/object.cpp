#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

// Object

Object::Object(Ref<Class> cls): _cls{cls}, _bits{cls->bitsize()} {}

Object::Object(Ref<Class> cls, BitVector&& bits):
    _cls{cls}, _bits{std::move(bits)} {
    assert(cls);
    assert(_bits.len() == cls->bitsize());
}

Object::~Object() {}

SPtr<Object> Object::copy() const {
    return make_s<Object>(_cls, _bits.copy());
}

ObjectView Object::view() { return {cls(), _bits.view()}; }

const ObjectView Object::view() const {
    return const_cast<Object*>(this)->view();
}

// ObjectView

ObjectView::ObjectView(Ref<Class> cls, BitVectorView bits):
    _cls{cls}, _bits{bits} {
    assert(_bits.len() == cls->bitsize());
}

ObjectView::operator bool() const {
    assert((bool)_cls == (bool)_bits);
    return _cls;
}

} // namespace ulam

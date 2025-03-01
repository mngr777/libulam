#include <cassert>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

// Object

Object::Object(Ref<Type> type): _BitStorage{type->bitsize()}, _type{type} {
    assert(type->is_object());
}

Object::Object(Ref<Type> type, BitVector&& bits_):
    _BitStorage{std::move(bits_)}, _type{type} {
    assert(_type && bits().len() == type->bitsize());
    assert(type->is_object());
}

Object::~Object() {}

SPtr<Object> Object::copy() const {
    return make_s<Object>(_type, bits().copy());
}

void Object::cast(Ref<Type> type) {
    assert(type->is_object());
    assert(_type->is_expl_castable_to(type));
    _type = type;
}

ObjectView Object::view() { return ObjectView{_type, bits().view()}; }

const ObjectView Object::view() const {
    return const_cast<Object*>(this)->view();
}

Ref<Class> Object::cls() const {
    return _type->as_class();
}

// ObjectView

ObjectView::ObjectView(Ref<Type> type, BitVectorView bits_):
    _BitStorageView{bits_}, _type{type} {
}

SPtr<Object> ObjectView::copy() const {
    return make_s<Object>(_type, bits().copy());
}

void ObjectView::cast(Ref<Type> type) {
    assert(type->is_object());
    assert(_type->is_expl_castable_to(type));
    _type = type;
}

ObjectView::operator bool() const {
    assert((bool)_type == (bits().len() == 0));
    return _type;
}

Ref<Class> ObjectView::cls() const {
    return _type->as_class();
}

} // namespace ulam

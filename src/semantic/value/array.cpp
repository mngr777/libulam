#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/array.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

namespace {
bitsize_t offset(Ref<Type> type, array_idx_t index) {
    assert(index != UnknownArrayIdx);
    return type->bitsize() * index;
}
} // namespace

// Array

Array::Array(Ref<ArrayType> array_type): Array{array_type->bitsize()} {}

Array::Array(Ref<Type> type, array_size_t size):
    Array{(bitsize_t)(type->bitsize() * size)} {}

ArrayView Array::view() { return ArrayView{bits().view()}; }

const ArrayView Array::view() const { return ArrayView{bits().view()}; }

RValue Array::load(Ref<Type> type, array_idx_t index) const {
    assert(type->bitsize() * (index + 1u) <= bits().len());
    return type->load(bits(), offset(type, index));
}

// ArrayAccess

ArrayAccess::ArrayAccess(
    ArrayView array_view, Ref<Type> type, array_idx_t index):
    _array_view{array_view}, _type{type}, _index{index} {
    assert(_array_view);
    assert(_type);
    assert(_index != UnknownArrayIdx);
    assert(_type->bitsize() * (_index + 1u) <= _array_view.bits().len());
}

BitsView ArrayAccess::item_bits_view() {
    return _array_view.bits().view(offset(_type, _index), _type->actual()->bitsize());
}

ArrayAccess ArrayAccess::item_array_access(array_idx_t index) {
    return ArrayAccess{
        item_array_view(), _type->actual()->as_array()->item_type(), index};
}

ArrayView ArrayAccess::item_array_view() {
    assert(_type->actual()->is_array());
    return ArrayView{item_bits_view()};
}

ObjectView ArrayAccess::item_object_view() {
    assert(_type->actual()->is_object());
    return ObjectView{_type, item_bits_view()};
}

RValue ArrayAccess::load() const {
    return _type->actual()->load(_array_view.bits(), offset(_type, _index));
}

void ArrayAccess::store(RValue&& rval) {
    _type->actual()->store(_array_view.bits(), offset(_type, _index), rval);
}

} // namespace ulam

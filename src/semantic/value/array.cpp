#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/array.hpp>

namespace ulam {

// Array

ArrayView Array::view() { return ArrayView{bits().view()}; }

const ArrayView Array::view() const { return ArrayView{bits().view()}; }

// ArrayAccess

ArrayAccess::ArrayAccess(
    ArrayView array_view, Ref<Type> type, array_idx_t index):
    _array_view{array_view}, _type{type}, _index{index} {
    assert(_array_view);
    assert(_type);
    assert(_index != UnknownArrayIdx);
    assert(_type->bitsize() * (_index + 1u) <= _array_view.bits().len());
}

RValue ArrayAccess::load() const {
    return _type->load(_array_view.bits(), _type->bitsize() * _index);
}

void ArrayAccess::store(RValue&& rval) {
    _type->store(_array_view.bits(), off(), rval);
}

BitVector::size_t ArrayAccess::off() const { return _type->bitsize() * _index; }

} // namespace ulam

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_storage.hpp>
#include <libulam/semantic/value/bit_vector.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

class ArrayType;
class ArrayView;
class ObjectView;
class RValue;
class Type;

class Array : public _BitStorage {
public:
    explicit Array(Ref<ArrayType> array_type);
    explicit Array(Ref<Type> type, array_size_t size);
    explicit Array(BitVector::size_t bitsize): _BitStorage{bitsize} {}
    explicit Array(BitVector&& bits): _BitStorage{std::move(bits)} {}

    Array copy() const { return Array{bits().copy()}; }

    ArrayView view();
    const ArrayView view() const;

    RValue load(Ref<Type> type, array_idx_t index) const;
};

class ArrayView : public _BitStorageView {
public:
    explicit ArrayView(BitVectorView bits): _BitStorageView(bits) {}
    ArrayView();
};

class ArrayAccess {
public:
    explicit ArrayAccess(
        ArrayView array_view, Ref<Type> type, array_idx_t index);
    explicit ArrayAccess(Array& array, Ref<Type> type, array_idx_t index):
        ArrayAccess{array.view(), type, index} {}

    Ref<Type> type() const { return _type; }

    array_idx_t index() const { return _index; }

    BitVectorView item_bits_view();
    ArrayAccess item_array_access(array_idx_t index);
    ArrayView item_array_view();
    ObjectView item_object_view();

    RValue load() const;
    void store(RValue&& rval);

private:
    ArrayView _array_view;
    Ref<Type> _type;
    array_idx_t _index;
};

} // namespace ulam

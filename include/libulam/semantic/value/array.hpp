#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_storage.hpp>
#include <libulam/semantic/value/bit_vector.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

class ArrayView;
class RValue;
class Type;

class Array : public _BitStorage {
public:
    explicit Array(BitVector::size_t bitsize): _BitStorage{bitsize} {}
    explicit Array(BitVector&& bits): _BitStorage{std::move(bits)} {}

    Array copy() const { return Array{bits().copy()}; }

    ArrayView view();
    const ArrayView view() const;
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

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }

    array_idx_t index() const { return _index; }

    RValue load() const;
    void store(RValue&& rval);

private:
    BitVector::size_t off() const;

    ArrayView _array_view;
    Ref<Type> _type;
    array_idx_t _index;
};

} // namespace ulam

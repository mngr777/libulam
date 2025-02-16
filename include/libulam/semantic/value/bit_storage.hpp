#pragma once
#include <libulam/semantic/value/bit_vector.hpp>
#include <utility>

namespace ulam {

class _BitStorage {
public:
    explicit _BitStorage(BitVector::size_t size): _bits{size} {}
    explicit _BitStorage(BitVector&& bits): _bits{std::move(bits)} {}
    _BitStorage(): _bits{0} {}

    _BitStorage(_BitStorage&&) = default;
    _BitStorage& operator=(_BitStorage&&) = default;

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    BitVector _bits;
};

class _BitStorageView {
public:
    explicit _BitStorageView(BitVectorView bits): _bits{bits} {}
    _BitStorageView() {}

    operator bool() const { return _bits; }

    BitVectorView bits() { return _bits; }
    const BitVectorView bits() const { return _bits; }

private:
    BitVectorView _bits;
};

} // namespace ulam

#pragma once
#include <libulam/semantic/value/bits.hpp>
#include <utility>

namespace ulam {

class _BitStorage {
public:
    explicit _BitStorage(bitsize_t size): _bits{size} {}
    explicit _BitStorage(Bits&& bits): _bits{std::move(bits)} {}
    _BitStorage(): _bits{0} {}

    _BitStorage(_BitStorage&&) = default;
    _BitStorage& operator=(_BitStorage&&) = default;

    Bits& bits() { return _bits; }
    const Bits& bits() const { return _bits; }

private:
    Bits _bits;
};

class _BitStorageView {
public:
    explicit _BitStorageView(BitsView bits): _bits{bits} {}
    _BitStorageView() {}

    operator bool() const { return _bits; }

    BitsView bits() { return _bits; }
    const BitsView bits() const { return _bits; }

private:
    BitsView _bits;
};

} // namespace ulam

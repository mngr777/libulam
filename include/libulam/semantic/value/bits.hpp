#pragma once
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

// TODO: optimization for shorter values
class Bits {
public:
    explicit Bits(BitVector::size_t size): _bits{size} {}
    explicit Bits(BitVector&& bits): _bits{std::move(bits)} {}

    std::uint16_t bitsize() const { return _bits.len(); }

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    BitVector _bits;
};

} // namespace ulam

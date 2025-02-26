#pragma once
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

// TODO: optimization for shorter values
class Bits {
public:
    explicit Bits(bitsize_t size): _bits{size} {}
    explicit Bits(BitVector&& bits): _bits{std::move(bits)} {}

    Bits(Bits&&) = default;
    Bits& operator=(Bits&&) = default;

    Bits copy() const { return Bits{_bits.copy()}; }

    bitsize_t bitsize() const { return _bits.len(); }

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    BitVector _bits;
};

} // namespace ulam

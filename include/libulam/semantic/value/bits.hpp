#pragma once
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

class Bits {
public:
    explicit Bits(BitVector::size_t size): _bits(size) {}

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    BitVector _bits;
};

} // namespace ulam

#pragma once
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

class Bits {
public:
    Bits(BitVector::size_t size): _bits(size) {}

    const BitVector& bits() { return _bits; }

private:
    BitVector _bits;
};

} // namespace ulam

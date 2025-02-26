#pragma once
#include <cassert>
#include <libulam/semantic/value/bit_vector.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

class Atom {
public:
    explicit Atom(): _bits{ULAM_ATOM_SIZE} {}
    explicit Atom(BitVector&& bits): _bits{std::move(bits)} {
        assert(bits.len() == ULAM_ATOM_SIZE);
    }

    Atom(Atom&&) = default;
    Atom& operator=(Atom&&) = default;

    Atom copy() const { return Atom{_bits.copy()}; }

    bitsize_t bitsize() const { return _bits.len(); }

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    BitVector _bits;
};

} // namespace ulam

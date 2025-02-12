#pragma once
#include <libulam/semantic/value/types.hpp>
#include <vector>

namespace ulam {

// Dynamically-sized simple version of MFM::BitVector

class _BitVector {
public:
    using size_t = Unsigned;
    using idx_t = Unsigned;
    using unit_t = Datum;
    using unit_idx_t = Unsigned;
};

class BitVector;

class BitVectorView : public _BitVector {
public:
    BitVectorView(BitVector& data, size_t off, size_t len);
    BitVectorView(BitVector& data);

    BitVectorView view() { return *this; }
    const BitVectorView view() const { return *this; }

    BitVectorView view(size_t off, size_t len) {
        return {_data, _off + off, len};
    }
    const BitVectorView view(size_t off, size_t len) const {
        return {const_cast<BitVector&>(_data), _off + off, len};
    }

    bool read_bit(idx_t idx) const;
    void write_bit(idx_t idx, bool bit);

    unit_t read(idx_t idx, size_t len) const;
    void write(idx_t idx, size_t len, unit_t value);

    size_t len() const { return _len; }

private:
    BitVector& _data;
    size_t _off;
    size_t _len;
};

class BitVector : public _BitVector {
public:
    static constexpr size_t UnitSize = sizeof(size_t) * 8;
    static constexpr Unsigned AtomSize = 96;
    static constexpr Unsigned Size8k = 8162;

    BitVector(size_t len): _len{len}, _bits{(len + UnitSize - 1) / UnitSize} {}

    BitVector(BitVector&& other) = default;
    BitVector& operator=(BitVector&&) = default;

    BitVectorView view() { return *this; }
    const BitVectorView view() const { return const_cast<BitVector&>(*this); }

    BitVectorView view(size_t off, size_t len) { return {*this, off, len}; }
    const BitVectorView view(size_t off, size_t len) const {
        return {const_cast<BitVector&>(*this), off, len};
    }

    bool read_bit(idx_t idx) const;
    void write_bit(idx_t idx, bool bit);

    unit_t read(idx_t idx, size_t len) const;
    void write(idx_t idx, size_t len, unit_t value);

    size_t len() const { return _len; }

private:
    unit_t read(unit_idx_t unit_idx, size_t start, size_t len) const;
    void write(unit_idx_t unit_idx, size_t start, size_t len, unit_t value);

    size_t _len;
    std::vector<unit_t> _bits;
};

} // namespace ulam

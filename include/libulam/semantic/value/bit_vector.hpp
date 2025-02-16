#pragma once
#include <functional>
#include <libulam/semantic/value/types.hpp>
#include <vector>

namespace ulam {

// Dynamically-sized simple version of MFM::BitVector

class _BitVector {
public:
    using size_t = Unsigned;
    using unit_t = Datum;
    using unit_idx_t = Unsigned;

    static constexpr size_t UnitSize = sizeof(unit_t) * 8;
    static constexpr Unsigned AtomSize = 96;
    static constexpr Unsigned Size8k = 8162;
};

class BitVector;

class BitVectorView : public _BitVector {
public:
    explicit BitVectorView(BitVector& data, size_t off, size_t len);
    explicit BitVectorView(BitVector& data);
    BitVectorView() {}

    BitVectorView view() { return *this; }
    const BitVectorView view() const { return *this; }

    BitVectorView view(size_t off, size_t len) {
        return BitVectorView{data(), _off + off, len};
    }
    const BitVectorView view(size_t off, size_t len) const {
        return BitVectorView{const_cast<BitVector&>(data()), _off + off, len};
    }

    bool read_bit(size_t idx) const;
    void write_bit(size_t idx, bool bit);

    unit_t read(size_t idx, size_t len) const;
    void write(size_t idx, size_t len, unit_t value);
    void write(size_t idx, const BitVectorView other);

    unit_t read_right(size_t len) const;

    size_t len() const { return _len; }

    BitVector copy() const;

    operator bool() const { return _data; }

    BitVectorView& operator&=(const BitVectorView& other);
    BitVectorView& operator|=(const BitVectorView& other);
    BitVectorView& operator^=(const BitVectorView& other);

    BitVector operator&(const BitVectorView& other) const;
    BitVector operator|(const BitVectorView& other) const;
    BitVector operator^(const BitVectorView& other) const;

private:
    using UnitBinOp = std::function<unit_t(unit_t, unit_t)>;

    void bin_op(const BitVectorView& other, UnitBinOp op);

    BitVector& data();
    const BitVector& data() const;

    BitVector* _data{};
    size_t _off;
    size_t _len;
};

class BitVector : public _BitVector {
public:
    explicit BitVector(size_t len):
        _len{len}, _bits((len + UnitSize - 1) / UnitSize, 0) {}

    // BitVector(BitVector&& other) = default;
    // BitVector& operator=(BitVector&&) = default;

    BitVectorView view() { return BitVectorView{*this}; }
    const BitVectorView view() const {
        return BitVectorView{const_cast<BitVector&>(*this)};
    }

    BitVectorView view(size_t off, size_t len) {
        return BitVectorView{*this, off, len};
    }
    const BitVectorView view(size_t off, size_t len) const {
        return BitVectorView{const_cast<BitVector&>(*this), off, len};
    }

    BitVector copy() const;

    bool read_bit(size_t idx) const;
    void write_bit(size_t idx, bool bit);

    unit_t read(size_t idx, size_t len) const;
    void write(size_t idx, size_t len, unit_t value);
    void write(size_t idx, const BitVectorView view);

    unit_t read_right(size_t len) const;

    size_t len() const { return _len; }

    BitVector& operator&=(const BitVector& other);
    BitVector& operator|=(const BitVector& other);
    BitVector& operator^=(const BitVector& other);

    BitVector operator&(const BitVector& other) const;
    BitVector operator|(const BitVector& other) const;
    BitVector operator^(const BitVector& other) const;

    BitVector& operator&=(const BitVectorView other);
    BitVector& operator|=(const BitVectorView other);
    BitVector& operator^=(const BitVectorView other);

    BitVector operator&(const BitVectorView other) const;
    BitVector operator|(const BitVectorView other) const;
    BitVector operator^(const BitVectorView other) const;

private:
    unit_t read(unit_idx_t unit_idx, size_t start, size_t len) const;
    void write(unit_idx_t unit_idx, size_t start, size_t len, unit_t value);

    size_t _len;
    std::vector<unit_t> _bits;
};

} // namespace ulam

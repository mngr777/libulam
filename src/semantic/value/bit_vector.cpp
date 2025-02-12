#include <cassert>
#include <libulam/semantic/value/bit_vector.hpp>
#include <limits>

// NOTE: keeping it simple for now
// TODO: (maybe) optimize, see MFM::BitVector impl

namespace ulam {
namespace {
static constexpr BitVector::unit_t UnitMax =
    std::numeric_limits<BitVector::unit_t>::max();

static constexpr BitVector::unit_t MSB = UnitMax & ~(UnitMax >> 1);

BitVector::unit_t make_mask(BitVector::size_t len, BitVector::size_t shift) {
    return ((len < BitVector::UnitSize) ? (1u << len) - 1 : -1) << shift;
}

BitVector::unit_idx_t to_unit_idx(BitVector::idx_t idx) {
    return idx / BitVector::UnitSize;
}

BitVector::size_t to_off(BitVector::idx_t idx) {
    return idx % BitVector::UnitSize;
}

} // namespace

// BitVectorView

BitVectorView::BitVectorView(BitVector& data, size_t off, size_t len):
    _data{data}, _off{off}, _len{len} {
    assert(off + len <= data.len());
}

BitVectorView::BitVectorView(BitVector& data):
    BitVectorView{data, 0, data.len()} {}

bool BitVectorView::read_bit(idx_t idx) const {
    assert(idx < _len);
    return _data.read_bit(idx);
}

void BitVectorView::write_bit(idx_t idx, bool bit) {
    assert(idx < _len);
    _data.write_bit(idx, bit);
}

BitVectorView::unit_t BitVectorView::read(idx_t idx, size_t len) const {
    assert(idx + len <= _len);
    return _data.read(idx, len);
}

void BitVectorView::write(idx_t idx, size_t len, unit_t value) {
    assert(idx + len <= _len);
    _data.write(idx, len, value);
}

// BitVector

bool BitVector::read_bit(idx_t idx) const {
    assert(idx < _len);
    return _bits[to_unit_idx(idx)] & (MSB >> to_off(idx));
}

void BitVector::write_bit(idx_t idx, bool bit) {
    assert(idx < _len);
    const unit_idx_t unit_idx = to_unit_idx(idx);
    const unit_t mask = MSB >> to_off(idx);
    if (bit) {
        _bits[unit_idx] |= mask;
    } else {
        _bits[unit_idx] &= ~mask;
    }
}

BitVector::unit_t BitVector::read(idx_t idx, size_t len) const {
    assert(idx < _len);
    assert(len <= UnitSize);
    const unit_idx_t unit_idx = to_unit_idx(idx);
    const size_t off = to_off(idx);
    if (off + len <= UnitSize)
        return read(unit_idx, off, len);
    assert(unit_idx + 1 < _bits.size());
    const size_t len_1 = UnitSize - off;
    const size_t len_2 = len - len_1;
    return (read(unit_idx, off, len_1) << len_2) | read(unit_idx + 1, 0, len_2);
}

void BitVector::write(idx_t idx, size_t len, unit_t value) {
    assert(idx < _len);
    assert(len <= UnitSize);
    const unit_idx_t unit_idx = to_unit_idx(idx);
    const size_t off = to_off(idx);
    if (off + len <= UnitSize) {
        write(unit_idx, off, len, value);
        return;
    }
    assert(unit_idx + 1 < _bits.size());
    const size_t len_1 = UnitSize - off;
    const size_t len_2 = len - len_1;
    write(unit_idx, off, len_1, value >> len_2);
    write(unit_idx + 1, 0, len_2, value << len_1);
}

BitVector::unit_t
BitVector::read(unit_idx_t unit_idx, size_t off, size_t len) const {
    assert(unit_idx < _bits.size());
    assert(off + len < UnitSize);
    if (len == 0)
        return 0;
    const size_t shift = UnitSize - (off + len);
    const unit_t mask = make_mask(len, 0);
    return (_bits[unit_idx] >> shift) & mask;
}

void BitVector::write(
    unit_idx_t unit_idx, size_t start, size_t len, unit_t value) {
    assert(unit_idx < _bits.size());
    assert(start + len < UnitSize);
    if (len == 0)
        return;
    const size_t shift = UnitSize - (start + len);
    const unit_t mask = make_mask(len, shift);
    _bits[unit_idx] = (_bits[unit_idx] & ~mask) | (value << shift);
}

} // namespace ulam

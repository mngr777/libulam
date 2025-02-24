#include <algorithm>
#include <cassert>
#include <libulam/semantic/value/bit_vector.hpp>
#include <limits>

// NOTE: keeping it simple for now
// TODO: (maybe) optimize, see MFM::BitVector impl, use variant<uint32_t,vector>
// TODO: naming: size/len, capitalize consts

namespace ulam {
namespace {
static constexpr BitVector::unit_t UnitMax =
    std::numeric_limits<BitVector::unit_t>::max();

static constexpr BitVector::unit_t MSB = UnitMax & ~(UnitMax >> 1);

BitVector::unit_t make_mask(BitVector::size_t len, BitVector::size_t shift) {
    return ((len < BitVector::UnitSize) ? (1u << len) - 1 : -1) << shift;
}

BitVector::unit_idx_t to_unit_idx(BitVector::size_t idx) {
    return idx / BitVector::UnitSize;
}

BitVector::size_t to_off(BitVector::size_t idx) {
    return idx % BitVector::UnitSize;
}

} // namespace

// BitVectorView

BitVectorView::BitVectorView(BitVector& data, size_t off, size_t len):
    _data{&data}, _off{off}, _len{len} {
    assert(off + len <= data.len());
}

BitVectorView::BitVectorView(BitVector& data):
    BitVectorView{data, 0, data.len()} {}

bool BitVectorView::read_bit(size_t idx) const {
    assert(idx < _len);
    return data().read_bit(_off + idx);
}

void BitVectorView::write_bit(size_t idx, bool bit) {
    assert(idx < _len);
    data().write_bit(_off + idx, bit);
}

BitVectorView::unit_t BitVectorView::read(size_t idx, size_t len) const {
    assert(idx + len <= _len);
    return data().read(_off + idx, len);
}

void BitVectorView::write(size_t idx, size_t len, unit_t value) {
    assert(idx + len <= _len);
    data().write(_off + idx, len, value);
}

void BitVectorView::write(size_t idx, const BitVectorView other) {
    assert(idx < _len);
    assert(idx + other.len() <= _len);
    for (size_t idx2 = 0; idx2 < other.len(); idx2 += UnitSize) {
        size_t size = std::min(UnitSize, other.len() - idx2);
        write(idx, size, other.read(idx2, size));
        idx += size;
    }
}

BitVectorView::unit_t BitVectorView::read_right(size_t len) const {
    assert(len <= _len);
    return data().read(_off + _len - len, len);
}

void BitVectorView::write_right(size_t len, unit_t value) {
    assert(len <= _len);
    data().write(_off + _len - len, len, value);
}

BitVector BitVectorView::copy() const {
    BitVector bv{len()};
    for (size_t off = 0; off < len(); off += UnitSize) {
        size_t size = (off + UnitSize > len()) ? len() - off : UnitSize;
        bv.write(off, size, read(off, size));
    }
    return bv;
}

bool BitVectorView::operator==(const BitVectorView& other) {
    if (_len != other._len)
        return false;
    for (unit_idx_t unit_idx = 0; unit_idx < _len / UnitSize; ++unit_idx) {
        size_t idx = unit_idx * UnitSize;
        if (read(idx, UnitSize) != other.read(idx, UnitSize))
            return false;
    }
    size_t rem = _len % UnitSize;
    return (rem == 0) || read_right(rem) == other.read_right(rem);
}

bool BitVectorView::operator!=(const BitVectorView& other) {
    return !operator==(other);
}

BitVectorView& BitVectorView::operator&=(const BitVectorView& other) {
    bin_op(other, std::bit_and<unit_t>{});
    return *this;
}

BitVectorView& BitVectorView::operator|=(const BitVectorView& other) {
    bin_op(other, std::bit_or<unit_t>{});
    return *this;
}

BitVectorView& BitVectorView::operator^=(const BitVectorView& other) {
    bin_op(other, std::bit_xor<unit_t>{});
    return *this;
}

BitVector BitVectorView::operator&(const BitVectorView& other) const {
    auto bv = copy();
    bv.view() &= other;
    return bv;
}

BitVector BitVectorView::operator|(const BitVectorView& other) const {
    auto bv = copy();
    bv.view() |= other;
    return bv;
}

BitVector BitVectorView::operator^(const BitVectorView& other) const {
    auto bv = copy();
    bv.view() ^= other;
    return bv;
}

void BitVectorView::bin_op(const BitVectorView& other, UnitBinOp op) {
    size_t off1 = 0; // offset from the right
    while (off1 < len()) {
        size_t size1 = UnitSize;
        off1 += UnitSize;
        if (off1 > len()) {
            size1 = len() + UnitSize - off1;
            off1 = len();
        }
        unit_t u1 = read(len() - off1, size1);
        unit_t u2 = 0;
        if (other.len() + size1 < off1) {
            size_t off2 = off1;
            size_t size2 = size1;
            if (off2 > other.len()) {
                size2 = other.len() + size1 - off1;
                off2 = other.len();
            }
            assert(off2 <= other.len());
            assert(size2 > 0);
            u2 = other.read(other.len() - off2, size2);
        }
        write(len() - off1, size1, op(u1, u2));
    }
}

BitVector& BitVectorView::data() {
    assert(_data);
    return *_data;
}

const BitVector& BitVectorView::data() const {
    assert(_data);
    return *_data;
}

// BitVector

BitVector BitVector::copy() const {
    BitVector bv{len()};
    bv._bits = _bits;
    return bv;
}

BitVectorView BitVector::view_right(size_t len) {
    assert(len <= _len);
    return BitVectorView{*this, _len - len, len};
}

const BitVectorView BitVector::view_right(size_t len) const {
    return const_cast<BitVector*>(this)->view_right(len);
}

bool BitVector::read_bit(size_t idx) const {
    assert(idx < _len);
    return _bits[to_unit_idx(idx)] & (MSB >> to_off(idx));
}

void BitVector::write_bit(size_t idx, bool bit) {
    assert(idx < _len);
    const unit_idx_t unit_idx = to_unit_idx(idx);
    const unit_t mask = MSB >> to_off(idx);
    if (bit) {
        _bits[unit_idx] |= mask;
    } else {
        _bits[unit_idx] &= ~mask;
    }
}

BitVector::unit_t BitVector::read(size_t idx, size_t len) const {
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

void BitVector::write(size_t idx, size_t len, unit_t value) {
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

void BitVector::write(size_t idx, const BitVectorView view_) {
    view().write(idx, view_);
}

BitVector::unit_t BitVector::read_right(size_t len) const {
    assert(len <= _len);
    return read(_len - len, len);
}

void BitVector::write_right(size_t len, unit_t value) {
    assert(len <= _len);
    return write(_len - len, len, value);
}

BitVector::unit_t
BitVector::read(unit_idx_t unit_idx, size_t off, size_t len) const {
    assert(unit_idx < _bits.size());
    assert(off + len <= UnitSize);
    if (len == 0)
        return 0;
    const size_t shift = UnitSize - (off + len);
    const unit_t mask = make_mask(len, 0);
    return (_bits[unit_idx] >> shift) & mask;
}

void BitVector::write(
    unit_idx_t unit_idx, size_t start, size_t len, unit_t value) {
    assert(unit_idx < _bits.size());
    assert(start + len <= UnitSize);
    if (len == 0)
        return;
    const size_t shift = UnitSize - (start + len);
    const unit_t mask = make_mask(len, shift);
    _bits[unit_idx] = (_bits[unit_idx] & ~mask) | (value << shift);
}

bool BitVector::operator==(const BitVector& other) const {
    return _len == other._len && _bits == other._bits;
}

bool BitVector::operator!=(const BitVector& other) const {
    return !operator==(other);
}

BitVector& BitVector::operator&=(const BitVector& other) {
    return operator&=(other.view());
}

BitVector& BitVector::operator|=(const BitVector& other) {
    return operator|=(other.view());
}

BitVector& BitVector::operator^=(const BitVector& other) {
    return operator^=(other.view());
}

BitVector& BitVector::operator<<=(size_t shift) {
    if (_len == 0 || shift == 0) {
        return *this;
    }
    if (shift >= _len) {
        clear();
        return *this;
    }
    const unit_idx_t ShiftUnits = shift / UnitSize;
    const size_t ShiftRem = shift % UnitSize;
    if (ShiftRem > 0) {
        const unit_t Mask = make_mask(ShiftRem, UnitSize - ShiftRem);
        auto to = _bits.begin();
        auto from = to + ShiftUnits;
        while (true) {
            *to = *from << ShiftRem;
            if (++from == _bits.end())
                break;
            *to |= (*from & Mask) >> (UnitSize - ShiftRem);
            ++to;
        }
    } else {
        std::copy(_bits.begin() + ShiftUnits, _bits.end(), _bits.begin());
    }
    assert((*(_bits.end() - 1) & ~last_unit_mask()) == 0);
    std::fill_n(_bits.end() - ShiftUnits, ShiftUnits, 0);
    return *this;
}

BitVector& BitVector::operator>>=(size_t shift) {
    if (_len == 0 || shift == 0) {
        return *this;
    }
    if (shift >= _len) {
        clear();
        return *this;
    }
    const unit_idx_t ShiftUnits = shift / UnitSize;
    const size_t ShiftRem = shift % UnitSize;
    if (ShiftRem > 0) {
        const unit_t Mask = make_mask(ShiftRem, 0);
        auto to = _bits.end() - 1;
        auto from = _bits.end() - 1 - ShiftUnits;
        while (true) {
            *to = *from >> ShiftRem;
            if (from == _bits.begin())
                break;
            --from;
            *to |= (*from & Mask) << (UnitSize - ShiftRem);
            --to;
        }
    } else {
        std::copy_backward(_bits.begin(), _bits.end() - ShiftUnits, _bits.end());
    }
    *(_bits.end() - 1) &= last_unit_mask();
    std::fill_n(_bits.begin(), ShiftUnits, 0);
    return *this;
}

BitVector BitVector::operator&(const BitVector& other) const {
    return operator&(other.view());
}

BitVector BitVector::operator|(const BitVector& other) const {
    return operator|(other.view());
}

BitVector BitVector::operator^(const BitVector& other) const {
    return operator^(other.view());
}

BitVector& BitVector::operator&=(const BitVectorView other) {
    view() &= other.view();
    return *this;
}

BitVector& BitVector::operator|=(const BitVectorView other) {
    view() |= other.view();
    return *this;
}

BitVector& BitVector::operator^=(const BitVectorView other) {
    view() ^= other.view();
    return *this;
}

BitVector BitVector::operator&(const BitVectorView other) const {
    bool copy_other = other.len() > len();
    BitVector bv{copy_other ? other.copy() : copy()};
    bv.view() &= copy_other ? view() : other.view();
    return bv;
}

BitVector BitVector::operator|(const BitVectorView other) const {
    bool copy_other = other.len() > len();
    BitVector bv{copy_other ? other.copy() : copy()};
    bv.view() |= copy_other ? view() : other.view();
    return bv;
}

BitVector BitVector::operator^(const BitVectorView other) const {
    bool copy_other = other.len() > len();
    BitVector bv{copy_other ? other.copy() : copy()};
    bv.view() ^= copy_other ? view() : other.view();
    return bv;
}

BitVector BitVector::operator<<(size_t shift) {
    auto bv = copy();
    bv <<= shift;
    return bv;
}

BitVector BitVector::operator>>(size_t shift) {
    auto bv = copy();
    bv >>= shift;
    return bv;
}

void BitVector::clear() { std::fill(_bits.begin(), _bits.end(), 0); }

BitVector::unit_t BitVector::last_unit_mask() const {
    size_t rem = _len % UnitSize;
    return (rem == 0) ? make_mask(UnitSize, 0) : make_mask(rem, UnitSize - rem);
}

} // namespace ulam

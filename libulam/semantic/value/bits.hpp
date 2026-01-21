#pragma once
#include <functional>
#include <libulam/semantic/value/types.hpp>
#include <ostream>

namespace ulam {

// Dynamically-sized simple version of MFM::BitVector

class _Bits {
public:
    using size_t = bitsize_t;
    using unit_t = Datum;
    using unit_idx_t = bitsize_t;

    static constexpr size_t UnitSize = sizeof(unit_t) * 8;
    static constexpr Unsigned AtomSize = 96;
    static constexpr Unsigned Size8k = 8162;
};

class Bits;

class BitsView : public _Bits {
public:
    explicit BitsView(Bits& data, size_t off, size_t len);
    explicit BitsView(Bits& data);
    BitsView() {}

    BitsView view() { return *this; }
    const BitsView view() const { return *this; }

    BitsView view(size_t off, size_t len) {
        return BitsView{data(), (size_t)(_off + off), len};
    }
    const BitsView view(size_t off, size_t len) const {
        return BitsView{const_cast<Bits&>(data()), (size_t)(_off + off), len};
    }

    bool read_bit(size_t idx) const;
    void write_bit(size_t idx, bool bit);

    unit_t read(size_t idx, size_t len) const;
    void write(size_t idx, size_t len, unit_t value);
    void write(size_t idx, const BitsView other);

    unit_t read_right(size_t len) const;
    void write_right(size_t len, unit_t value);

    size_t len() const { return _len; }
    bool empty() const;

    Bits copy() const;

    operator bool() const { return _data; }

    bool operator==(const BitsView& other);
    bool operator!=(const BitsView& other);

    BitsView& operator&=(const BitsView& other);
    BitsView& operator|=(const BitsView& other);
    BitsView& operator^=(const BitsView& other);

    Bits operator&(const BitsView& other) const;
    Bits operator|(const BitsView& other) const;
    Bits operator^(const BitsView& other) const;

    void write_hex(std::ostream& out) const;
    std::string hex() const;

private:
    using UnitBinOp = std::function<unit_t(unit_t, unit_t)>;

    void bin_op(const BitsView& other, UnitBinOp op);

    Bits& data();
    const Bits& data() const;

    Bits* _data{};
    size_t _off;
    size_t _len;
};

class Bits : public _Bits {
public:
    explicit Bits(size_t len = 0);
    ~Bits();

    Bits(Bits&& other);
    Bits& operator=(Bits&&);

    BitsView view() { return BitsView{*this}; }
    const BitsView view() const {
        return BitsView{const_cast<Bits&>(*this)};
    }

    BitsView view(size_t off, size_t len) {
        return BitsView{*this, off, len};
    }
    const BitsView view(size_t off, size_t len) const {
        return BitsView{const_cast<Bits&>(*this), off, len};
    }

    BitsView view_right(size_t len);
    const BitsView view_right(size_t len) const;

    Bits copy() const;

    bool read_bit(size_t idx) const;
    void write_bit(size_t idx, bool bit);

    unit_t read(size_t idx, size_t len) const;
    void write(size_t idx, size_t len, unit_t value);
    void write(size_t idx, const BitsView view);

    unit_t read_right(size_t len) const;
    void write_right(size_t len, unit_t value);

    size_t len() const { return _len; }
    bool empty() const;

    void flip();

    bool operator==(const Bits& other) const;
    bool operator!=(const Bits& other) const;

    Bits& operator&=(const Bits& other);
    Bits& operator|=(const Bits& other);
    Bits& operator^=(const Bits& other);
    Bits& operator<<=(size_t shift);
    Bits& operator>>=(size_t shift);

    Bits operator&(const Bits& other) const;
    Bits operator|(const Bits& other) const;
    Bits operator^(const Bits& other) const;
    Bits operator<<(size_t shift);
    Bits operator>>(size_t shift);

    Bits& operator&=(const BitsView other);
    Bits& operator|=(const BitsView other);
    Bits& operator^=(const BitsView other);

    Bits operator&(const BitsView other) const;
    Bits operator|(const BitsView other) const;
    Bits operator^(const BitsView other) const;

    void write_hex(std::ostream& out) const;
    std::string hex() const;

private:
    unit_t read(unit_idx_t unit_idx, size_t start, size_t len) const;
    void write(unit_idx_t unit_idx, size_t start, size_t len, unit_t value);

    void clear();
    unit_t last_unit_mask() const;

    void init_storage();
    void destroy_storage();
    bool is_storage_dynamic() const;
    unit_idx_t storage_size() const;

    unit_t* storage();
    const unit_t* storage() const;

    size_t _len;

    union {
        unit_t array[1];
        unit_t* ptr;
    } _storage;
};

} // namespace ulam

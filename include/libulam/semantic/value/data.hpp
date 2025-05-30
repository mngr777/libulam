#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/types.hpp>
#include <memory>

namespace ulam {

class Data;
using DataPtr = SPtr<Data>;
using ConstDataPtr = SPtr<const Data>;

class DataView;
class Prop;
class RValue;
class Type;

class Data : public std::enable_shared_from_this<Data> {
public:
    Data(Ref<Type> type, Bits&& bits);
    explicit Data(Ref<Type>);

    Data(Data&&) = delete;
    Data& operator=(Data&&) = delete;

    DataPtr copy() const;

    DataView view();
    const DataView view() const;

    DataView as(Ref<Type> type);
    const DataView as(Ref<Type> type) const;

    DataView array_item(array_idx_t idx);
    const DataView array_item(array_idx_t idx) const;

    DataView prop(Ref<Prop> prop_);
    const DataView prop(Ref<Prop> prop_) const;

    DataView atom_of();
    const DataView atom_of() const;

    Ref<Type> type() const;

    Bits& bits() { return _bits; }
    const Bits& bits() const { return _bits; }

private:
    Ref<Type> _type;
    Bits _bits;
};

class DataView {
public:
    DataView(
        DataPtr storage,
        Ref<Type> type,
        bitsize_t off,
        bitsize_t atom_off = NoBitsize,
        Ref<Type> atom_type = Ref<Type>{});

    DataView() {}

    operator bool() const { return _storage.get(); }

    void store(RValue&& rval);
    RValue load() const;

    DataPtr storage() { return _storage; }
    ConstDataPtr storage() const { return _storage; }

    DataView as(Ref<Type> view_type);
    const DataView as(Ref<Type> view_type) const;

    DataView array_item(array_idx_t idx);
    const DataView array_item(array_idx_t idx) const;

    DataView prop(Ref<Prop> prop_);
    const DataView prop(Ref<Prop> prop_) const;

    DataView atom_of();
    const DataView atom_of() const;

    bitsize_t position_of() const;

    Ref<Type> type(bool real = false) const;

    BitsView bits();
    const BitsView bits() const;

private:
    void set_view_type(Ref<Type> view_type);
    Ref<Type> dyn_type() const;

    DataPtr _storage{};
    Ref<Type> _type{};
    Ref<Type> _view_type{};
    bitsize_t _off{};
    struct {
        bitsize_t off{NoBitsize};
        Ref<Type> type{};
    } _atom;
};

} // namespace ulam

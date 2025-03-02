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
    friend DataView;

public:
    Data(Ref<Type> type, Bits&& bits);
    explicit Data(Ref<Type>);

    DataView view();
    const DataView view() const;

    DataView array_item(array_idx_t idx);
    const DataView array_item(array_idx_t idx) const;

    DataView prop(Ref<Prop> prop_);
    const DataView prop(Ref<Prop> prop_) const;

    bool is_array() const;
    bool is_object() const;
    bool is_class() const;

    Ref<Type> type() const { return _type; }

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
        bitsize_t atom_off = NoBitsize);

    void store(RValue&& rval);
    RValue load() const;

    DataPtr storage() { return _storage; }
    ConstDataPtr storage() const { return _storage; }

    DataView array_item(array_idx_t idx);
    const DataView array_item(array_idx_t idx) const;

    DataView prop(Ref<Prop> prop_);
    const DataView prop(Ref<Prop> prop_) const;

    bool is_array() const;
    bool is_object() const;
    bool is_class() const;

    Ref<Type> type() const { return _type; }

private:
    DataPtr _storage;
    Ref<Type> _type;
    bitsize_t _off;
    bitsize_t _atom_off;
};

} // namespace ulam

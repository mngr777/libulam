#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

// Data

Data::Data(Ref<Type> type, Bits&& bits): _type{type}, _bits{std::move(bits)} {
    assert(is_array() || is_object());
}

Data::Data(Ref<Type> type): Data{type, Bits{type->bitsize()}} {}

DataPtr Data::copy() const { return make_s<Data>(_type, _bits.copy()); }

DataView Data::view() { return {shared_from_this(), _type, 0}; }

const DataView Data::view() const { return const_cast<Data*>(this)->view(); }

DataView Data::array_item(array_idx_t idx) {
    assert(is_array());
    auto array = _type->non_alias()->as_array();
    return {shared_from_this(), array->item_type(), array->item_off(idx)};
}

const DataView Data::array_item(array_idx_t idx) const {
    return const_cast<Data*>(this)->array_item(idx);
}

DataView Data::prop(Ref<Prop> prop_) {
    assert(is_class());
    auto cls = _type->canon()->as_class();
    return {shared_from_this(), prop_->type(), prop_->data_off_in(cls)};
}

const DataView Data::prop(Ref<Prop> prop_) const {
    return const_cast<Data*>(this)->prop(prop_);
}

bool Data::is_array() const { return _type->canon()->is_array(); }

bool Data::is_object() const { return _type->canon()->is_object(); }

bool Data::is_class() const { return _type->canon()->is_class(); }

// DataView

DataView::DataView(
    DataPtr storage, Ref<Type> type, bitsize_t off, bitsize_t atom_off):
    _storage{storage}, _type{type}, _off{off}, _atom_off{atom_off} {
    if (_atom_off == NoBitsize && type->canon()->is_atom())
        _atom_off = off;
}

void DataView::store(RValue&& rval) {
    _type->canon()->store(_storage->bits(), _off, std::move(rval));
}

RValue DataView::load() const {
    return _type->canon()->load(_storage->bits(), _off);
}

DataView DataView::array_item(array_idx_t idx) {
    assert(is_array());
    auto array = _type->non_alias()->as_array();
    bitsize_t off = _off + array->item_off(idx);
    return {_storage, array->item_type(), off, _atom_off};
}

const DataView DataView::array_item(array_idx_t idx) const {
    return const_cast<DataView*>(this)->array_item(idx);
}

DataView DataView::prop(Ref<Prop> prop_) {
    assert(is_class());
    auto cls = _type->canon()->as_class();
    return {_storage, prop_->type(), prop_->data_off_in(cls)};
}

const DataView DataView::prop(Ref<Prop> prop_) const {
    return const_cast<DataView*>(this)->prop(prop_);
}

bool DataView::is_array() const { return _type->canon()->is_array(); }

bool DataView::is_object() const { return _type->canon()->is_object(); }

bool DataView::is_class() const { return _type->canon()->is_class(); }

BitsView DataView::bits() {
    return _storage->bits().view(_off, _type->bitsize());
}

const BitsView DataView::bits() const {
    return _storage->bits().view(_off, _type->bitsize());
}

} // namespace ulam

#include "libulam/semantic/type/builtin_type_id.hpp"
#include "libulam/semantic/value/types.hpp"
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

DataView Data::as(Ref<Type> type) { return view().as(type); }

const DataView Data::as(Ref<Type> type) const { return view().as(type); }

DataView Data::array_item(array_idx_t idx) {
    auto array = _type->as_array();
    return {shared_from_this(), array->item_type(), array->item_off(idx)};
}

const DataView Data::array_item(array_idx_t idx) const {
    return const_cast<Data*>(this)->array_item(idx);
}

DataView Data::prop(Ref<Prop> prop_) {
    auto cls = _type->as_class();
    return {shared_from_this(), prop_->type(), prop_->data_off_in(cls)};
}

const DataView Data::prop(Ref<Prop> prop_) const {
    return const_cast<Data*>(this)->prop(prop_);
}

DataView Data::atom_of() { return is_atom() ? view() : DataView{}; }

const DataView Data::atom_of() const {
    return const_cast<Data*>(this)->atom_of();
}

bool Data::is_array() const { return _type->is_array(); }
bool Data::is_object() const { return _type->is_object(); }
bool Data::is_atom() const { return _type->is_atom(); }
Bool Data::is_class() const { return _type->is_class(); }

// DataView

DataView::DataView(
    DataPtr storage,
    Ref<Type> type,
    bitsize_t off,
    Ref<Type> view_type,
    bitsize_t atom_off,
    Ref<Type> atom_type):
    _storage{storage},
    _type{type},
    _off{off},
    _view_type{view_type ? view_type : type},
    _atom{atom_off, atom_type} {

    assert(
        _view_type->is_same(_type) ||
        /* viewing class as base */
        (_type->is_class() && _view_type->is_class() &&
         _view_type->as_class()->is_base_of(_type->as_class())));

    if (_atom.off == NoBitsize && _view_type->is_atom()) {
        _atom.off = off;
        _atom.type = _view_type;
    }
}

void DataView::store(RValue&& rval) {
    _type->store(_storage->bits(), _off, std::move(rval));
}

RValue DataView::load() const { return _type->load(_storage->bits(), _off); }

DataView DataView::as(Ref<Type> view_type) {
    assert(view_type);
    auto type = _type;
    /*
      viewing Atom as element or vice versa, or element as other element;
      there's no no need to keep the original type in this case
     */
    if (view_type->is_atom() && type->is_atom())
        type = view_type;
    return {_storage, type, _off, view_type, _atom.off, _atom.type};
}

const DataView DataView::as(Ref<Type> type) const {
    return const_cast<DataView*>(this)->as(type);
}

DataView DataView::array_item(array_idx_t idx) {
    assert(is_array());
    auto array = _view_type->as_array();
    bitsize_t off = _off + array->item_off(idx);
    return {_storage,    array->item_type(), off,
            Ref<Type>{}, _atom.off,          _atom.type};
}

const DataView DataView::array_item(array_idx_t idx) const {
    return const_cast<DataView*>(this)->array_item(idx);
}

DataView DataView::prop(Ref<Prop> prop_) {
    Ref<Class> cls{};
    if (_type->is_class()) {
        cls = _type->as_class();
        assert(_view_type->as_class()->is_same_or_base_of(_type->as_class()));
    } else {
        assert(_type->is(AtomId));
        cls = _view_type->as_class();
    }
    return {_storage,
            prop_->type(),
            (bitsize_t)(_off + prop_->data_off_in(cls)),
            prop_->type(),
            _atom.off,
            _atom.type};
}

const DataView DataView::prop(Ref<Prop> prop_) const {
    return const_cast<DataView*>(this)->prop(prop_);
}

DataView DataView::atom_of() {
    if (_atom.off == NoBitsize)
        return {};
    assert(_atom.type);
    return {_storage, _atom.type, _atom.off, _atom.type, _atom.off, _atom.type};
}

const DataView DataView::atom_of() const {
    return const_cast<DataView*>(this)->atom_of();
}

bool DataView::is_array() const { return _view_type->is_array(); }
bool DataView::is_object() const { return _view_type->is_object(); }
bool DataView::is_atom() const { return _view_type->is_atom(); }
Bool DataView::is_class() const { return _view_type->is_class(); }

BitsView DataView::bits() {
    return _storage->bits().view(_off, _type->bitsize());
}

const BitsView DataView::bits() const {
    return _storage->bits().view(_off, _type->bitsize());
}

} // namespace ulam

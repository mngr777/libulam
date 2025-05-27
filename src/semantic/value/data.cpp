#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

// Data

Data::Data(Ref<Type> type, Bits&& bits): _type{}, _bits{std::move(bits)} {
    assert(type->is_array() || type->is_object());
    if (type->is_atom())
        type = type->builtins().atom_type();
    _type = type;
}

Data::Data(Ref<Type> type): Data{type, Bits{type->bitsize()}} {}

DataPtr Data::copy() const { return make_s<Data>(_type, _bits.copy()); }

DataView Data::view() { return {shared_from_this(), _type, 0}; }

const DataView Data::view() const { return const_cast<Data*>(this)->view(); }

DataView Data::as(Ref<Type> type) { return view().as(type); }

const DataView Data::as(Ref<Type> type) const { return view().as(type); }

DataView Data::array_item(array_idx_t idx) {
    auto array = type()->as_array();
    return {shared_from_this(), array->item_type(), array->item_off(idx)};
}

const DataView Data::array_item(array_idx_t idx) const {
    return const_cast<Data*>(this)->array_item(idx);
}

DataView Data::prop(Ref<Prop> prop_) {
    auto cls = type()->as_class();
    return {shared_from_this(), prop_->type(), prop_->data_off_in(cls)};
}

const DataView Data::prop(Ref<Prop> prop_) const {
    return const_cast<Data*>(this)->prop(prop_);
}

DataView Data::atom_of() { return type()->is_atom() ? view() : DataView{}; }

const DataView Data::atom_of() const {
    return const_cast<Data*>(this)->atom_of();
}

Ref<Type> Data::type() const {
    return _type->is(AtomId)
               ? dynamic_cast<AtomType*>(_type)->data_type(_bits.view(), 0)
               : _type;
}

// DataView

DataView::DataView(
    DataPtr storage,
    Ref<Type> type,
    bitsize_t off,
    bitsize_t atom_off,
    Ref<Type> atom_type):
    _storage{storage}, _type{}, _off{off}, _atom{atom_off, atom_type} {

    if (type->is_atom())
        type = type->builtins().atom_type();
    _type = type;

    if (_atom.off == NoBitsize && _type->is_atom()) {
        _atom.off = off;
        _atom.type = _type;
    }
}

void DataView::store(RValue&& rval) {
    auto type = dyn_type();
    if (!_view_type || _view_type->is_same(type)) {
        type->store(_storage->bits(), _off, std::move(rval));
        return;
    }
    assert(_view_type->is_expl_castable_to(type));
    auto val = _view_type->cast_to(_type, Value{std::move(rval)});
    type->store(_storage->bits(), _off, val.move_rvalue());
}

RValue DataView::load() const {
    auto rval = _type->load(_storage->bits(), _off);
    auto type = dyn_type();
    if (_view_type && !_view_type->is_same(type)) {
        assert(type->is_expl_castable_to(_view_type));
        auto val = type->cast_to(_view_type, Value{std::move(rval)});
        return val.move_rvalue();
    }
    return rval;
}

DataView DataView::as(Ref<Type> view_type) {
    DataView view{*this};
    view.set_view_type(view_type);
    return view;
}

const DataView DataView::as(Ref<Type> type) const {
    return const_cast<DataView*>(this)->as(type);
}

DataView DataView::array_item(array_idx_t idx) {
    assert(type()->is_array());
    auto type_ = type();
    auto array_type = type_->as_array();
    auto item_type = type_->as_array()->item_type();
    bitsize_t off = _off + array_type->item_off(idx);
    return {_storage, item_type, off, _atom.off, _atom.type};
}

const DataView DataView::array_item(array_idx_t idx) const {
    return const_cast<DataView*>(this)->array_item(idx);
}

DataView DataView::prop(Ref<Prop> prop_) {
    // NOTE: it is possible use a property of unrelated (element) class by
    // rewriting `atomof` with an element or raw Atom (t3909)
    auto type_ = dyn_type();
    bitsize_t off = _off + prop_->data_off();
    assert(
        type_->is_class() || (type_->is(AtomId) && prop_->cls()->is_element()));
    if (type_->is_class()) {
        auto cls = type_->as_class();
        assert(
            prop_->cls()->is_same_or_base_of(cls) ||
            (cls->is_element() && prop_->cls()->is_element()));
        if (prop_->cls()->is_base_of(cls))
            off = _off + prop_->data_off_in(cls);
    }
    auto type = prop_->type();
    return {_storage, type, off, _atom.off, _atom.type};
}

const DataView DataView::prop(Ref<Prop> prop_) const {
    return const_cast<DataView*>(this)->prop(prop_);
}

DataView DataView::atom_of() {
    if (_atom.off == NoBitsize)
        return {};
    assert(_atom.type);
    return {_storage, _atom.type, _atom.off, _atom.off, _atom.type};
}

const DataView DataView::atom_of() const {
    return const_cast<DataView*>(this)->atom_of();
}

bitsize_t DataView::position_of() const {
    // TODO
    return _off;
}

Ref<Type> DataView::type(bool real) const {
    return (!real && _view_type) ? _view_type : dyn_type();
}

BitsView DataView::bits() {
    return _storage->bits().view(_off, _type->bitsize());
}

const BitsView DataView::bits() const {
    return _storage->bits().view(_off, _type->bitsize());
}

void DataView::set_view_type(Ref<Type> view_type) {
    assert(dyn_type()->is_expl_refable_as(view_type, Value{RValue{}}));
    _view_type = view_type;
}

Ref<Type> DataView::dyn_type() const {
    return _type->is(AtomId)
               ? dynamic_cast<AtomType*>(_type)->data_type(bits(), 0)
               : _type;
}

} // namespace ulam

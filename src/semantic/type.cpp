#include "libulam/semantic/type/conv.hpp"
#include "libulam/semantic/value/types.hpp"
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/array.hpp>

namespace ulam {

// Type

Type::~Type() {}

RValue Type::construct() { assert(false); }

RValue Type::load(const BitVector& data, BitVector::size_t off) const {
    return load(data.view(), off);
}

void Type::store(
    BitVector& data, BitVector::size_t off, const RValue& rval) const {
    store(data.view(), off, rval);
}

RValue Type::load(const BitVectorView data, BitVector::size_t off) const {
    assert(false);
}

void Type::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(false);
}

bool Type::is_canon() const { return canon() == this; }

Ref<ArrayType> Type::array_type(array_size_t size) {
    auto it = _array_types.find(size);
    if (it != _array_types.end())
        return ref(it->second);
    auto type = make_array_type(size);
    auto type_ref = ref(type);
    _array_types[size] = std::move(type);
    return type_ref;
}

Ref<RefType> Type::ref_type() {
    if (!_ref_type)
        _ref_type = make_ref_type();
    return ref(_ref_type);
}

TypedValue Type::type_op(TypeOp op) { assert(false); }

bool Type::is_same(Ref<const Type> type) const {
    return canon() == type->canon();
}

conv_cost_t Type::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return is_same(type) ? 0 : MaxConvCost;
}

RValue Type::cast_to(Ref<Type> type, RValue&& rval) { assert(false); }

Ptr<ArrayType> Type::make_array_type(array_size_t size) {
    return make<ArrayType>(_id_gen, this, size);
}

Ptr<RefType> Type::make_ref_type() { return make<RefType>(_id_gen, this); }

// AliasType

str_id_t AliasType::name_id() const { return _node->alias_id(); }

bitsize_t AliasType::bitsize() const {
    assert(_canon);
    return _canon->bitsize();
}

Ref<ast::TypeName> AliasType::type_name() {
    assert(_node->type_name());
    return _node->type_name();
}

Ref<ast::TypeExpr> AliasType::type_expr() {
    assert(_node->type_expr());
    return _node->type_expr();
}

void AliasType::set_aliased(Ref<Type> type) {
    assert(type);
    assert(!_aliased);
    _aliased = type;
    _canon = type->canon();
    _non_alias = type->non_alias();
}

Ptr<ArrayType> AliasType::make_array_type(array_size_t size) {
    auto type = Type::make_array_type(size);
    assert(_canon);
    type->set_canon(_canon->array_type(size));
    return type;
}

Ptr<RefType> AliasType::make_ref_type() {
    auto type = Type::make_ref_type();
    assert(_canon);
    type->set_canon(_canon->ref_type());
    return type;
}

// ArrayType

ArrayType::ArrayType(
    TypeIdGen* id_gen,
    Ref<Type> item_type,
    array_size_t array_size,
    Ref<ArrayType> canon):
    Type{id_gen}, _item_type{item_type}, _array_size{array_size}, _canon{this} {
    assert(array_size != UnknownArraySize);
}

bitsize_t ArrayType::bitsize() const {
    assert(_item_type);
    return _array_size * _item_type->bitsize();
}

RValue ArrayType::construct() { return RValue{Array{bitsize()}}; }

// TODO: use construct(), make it const
RValue ArrayType::load(const BitVectorView data, BitVector::size_t off) const {
    Array array{bitsize()};
    array.bits().write(0, data.view(off, bitsize()));
    return RValue{std::move(array)};
}

void ArrayType::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(rval.is<Array>());
    data.write(off, rval.get<Array>().bits().view());
}

void ArrayType::set_canon(Ref<ArrayType> canon) {
    assert(canon);
    assert(canon->item_type() == item_type()->canon());
    assert(canon->array_size() == array_size());
    _canon = canon;
}

// RefType

bitsize_t RefType::bitsize() const {
    assert(_refd);
    return _refd->bitsize();
}

void RefType::set_canon(Ref<RefType> canon) {
    assert(canon);
    assert(canon->refd() == refd()->canon());
    _canon = canon;
}

} // namespace ulam

#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/array.hpp>

namespace ulam {

// Type

Type::~Type() {}

RValue Type::construct() const { assert(false); }

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

bool Type::is_actual() const {
    return actual() == this;
}

Ref<Type> Type::actual() {
    return canon()->deref();
}

Ref<const Type> Type::actual() const {
    return canon()->deref();
}

bool Type::is_canon() const { return canon() == this; }

bool Type::is(BuiltinTypeId id) const {
    assert(id != NoBuiltinTypeId);
    return bi_type_id() == id;
}

bool Type::is_object() const { return is_class() || is(AtomId); }

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

TypedValue Type::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::SizeOf:
        return TypedValue{
            builtins().type(UnsignedId), Value{RValue{(Unsigned)bitsize()}}};
    default:
        return {};
    }
}

bool Type::is_same(Ref<const Type> type) const {
    return canon() == type->canon();
}

bool Type::is_same_actual(Ref<const Type> type) const {
    return actual() == type->actual();
}

bool Type::is_expl_castable_to(Ref<const Type> type) const {
    return is_castable_to(type, true);
}

bool Type::is_impl_castable_to(Ref<const Type> type) const {
    return is_castable_to(type, false);
}

bool Type::is_castable_to(Ref<const Type> type, bool expl) const {
    return false;
}

bool Type::is_impl_castable_to(BuiltinTypeId bi_type_id) const {
    return is_castable_to(bi_type_id, false);
}

bool Type::is_expl_castable_to(BuiltinTypeId bi_type_id) const {
    return is_castable_to(bi_type_id, true);
}

bool Type::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    return false;
}

bool Type::is_impl_castable_to(Ref<const Type> type, const Value& val) const {
    return is_impl_castable_to(type);
}

bool Type::is_impl_castable_to(
    BuiltinTypeId bi_type_id, const Value& val) const {
    return is_impl_castable_to(bi_type_id);
}

conv_cost_t Type::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return is_same(type) ? 0 : MaxConvCost;
}

RValue Type::cast_to(Ref<const Type> type, RValue&& rval) { assert(false); }

Ptr<ArrayType> Type::make_array_type(array_size_t size) {
    return make<ArrayType>(_builtins, _id_gen, this, size);
}

Ptr<RefType> Type::make_ref_type() {
    return make<RefType>(_builtins, _id_gen, this);
}

// AliasType

AliasType::AliasType(
    Builtins& builtins, TypeIdGen* id_gen, Ref<ast::TypeDef> node):
    UserType{builtins, id_gen}, _node(node) {
    assert(!node->alias_type());
    if (has_id())
        node->set_alias_type(this);
}

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

Ref<Type> AliasType::deref() { assert(false); }

Ref<const Type> AliasType::deref() const { assert(false); }

Ptr<ArrayType> AliasType::make_array_type(array_size_t size) {
    assert(_canon);
    auto type = Type::make_array_type(size);
    assert(type->canon() == _canon->array_type(size));
    return type;
}

Ptr<RefType> AliasType::make_ref_type() {
    assert(_canon);
    auto type = Type::make_ref_type();
    assert(type->canon() == _canon->ref_type());
    return type;
}

// ArrayType

ArrayType::ArrayType(
    Builtins& builtins,
    TypeIdGen* id_gen,
    Ref<Type> item_type,
    array_size_t size):
    Type{builtins, id_gen},
    _item_type{item_type},
    _array_size{size},
    _canon{
        item_type->is_canon() ? this : item_type->canon()->array_type(size)} {
    assert(size != UnknownArraySize);
}

bitsize_t ArrayType::bitsize() const {
    assert(_item_type);
    return _array_size * _item_type->bitsize();
}

RValue ArrayType::construct() const { return RValue{Array{bitsize()}}; }

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

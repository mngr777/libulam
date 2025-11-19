#include "libulam/semantic/value/types.hpp"
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

// Type

Type::~Type() {}

RValue Type::construct() { assert(false); }

RValue Type::load(const Bits& data, bitsize_t off) {
    return load(data.view(), off);
}

void Type::store(Bits& data, bitsize_t off, const RValue& rval) {
    store(data.view(), off, rval);
}

RValue Type::load(const BitsView data, bitsize_t off) { assert(false); }

void Type::store(BitsView data, bitsize_t off, const RValue& rval) {
    assert(false);
}

bool Type::is_actual() const { return actual() == this; }

Ref<Type> Type::actual() { return canon()->deref(); }

Ref<const Type> Type::actual() const { return canon()->deref(); }

bool Type::is_canon() const { return canon() == this; }

bool Type::is(BuiltinTypeId id) const {
    assert(id != NoBuiltinTypeId);
    return bi_type_id() == id;
}

bool Type::is_object() const { return is(AtomId) || is_class(); }

bool Type::is_atom() const {
    return is(AtomId) || (is_class() && as_class()->is_element());
}

Ref<PrimType> Type::as_prim() {
    assert(_as_prim());
    return _as_prim();
}

Ref<const PrimType> Type::as_prim() const {
    assert(_as_prim());
    return _as_prim();
}

Ref<Class> Type::as_class() {
    assert(_as_class());
    return _as_class();
}

Ref<const Class> Type::as_class() const {
    assert(_as_class());
    return _as_class();
}

Ref<AliasType> Type::as_alias() {
    assert(_as_alias());
    return _as_alias();
}

Ref<const AliasType> Type::as_alias() const {
    assert(_as_alias());
    return _as_alias();
}

Ref<ArrayType> Type::as_array() {
    assert(_as_array());
    return _as_array();
}
Ref<const ArrayType> Type::as_array() const {
    assert(_as_array());
    return _as_array();
}

Ref<RefType> Type::as_ref() {
    assert(_as_ref());
    return _as_ref();
}

Ref<const RefType> Type::as_ref() const {
    assert(_as_ref());
    return _as_ref();
}

TypedValue Type::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::SizeOf: {
        return TypedValue{
            builtins().type(UnsignedId),
            Value{RValue{(Unsigned)bitsize(), true}}};
    }
    case TypeOp::ConstantOf:
        // TODO: store constant in Program?
    case TypeOp::InstanceOf: {
        if (!is_constructible())
            return {};
        auto rval = construct();
        rval.set_is_consteval(true);
        return {this, Value{std::move(rval)}};
    }
    case TypeOp::ClassIdOf: {
        Unsigned id = is_class() ? as_class()->id() : NoTypeId;
        return TypedValue{builtins().type(UnsignedId), Value{RValue{id, true}}};
    }
    default:
        return {};
    }
}

TypedValue Type::type_op(TypeOp op, Value& val) {
    switch (op) {
    case TypeOp::AtomOf: {
        LValue lval;
        auto type = builtins().atom_type()->ref_type();
        if (val.empty()) {
            lval.set_is_xvalue(false);
            return {type, Value{std::move(lval)}};
        }
        lval = val.atom_of();
        if (lval.empty())
            return {};
        assert(lval.type()->is_atom());
        return {type, Value{lval}};
    }
    case TypeOp::PositionOf: {
        if (val.empty())
            return {};
        auto pos = val.position_of();
        if (pos == NoBitsize)
            return {};
        auto uns_type = builtins().unsigned_type();
        RValue rval{(Unsigned)pos};
        rval.set_is_consteval(true);
        return {uns_type, Value{std::move(rval)}};
    }
    default:
        return type_op(op);
    }
}

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

bool Type::is_same(Ref<const Type> type) const {
    return canon() == type->canon();
}

bool Type::is_same_actual(Ref<const Type> type) const {
    return actual() == type->actual();
}

Ref<Type> Type::common(Ref<Type> type) {
    if (is_same(type))
        return this;
    if (is_ref() != type->is_ref())
        return deref()->common(type->deref());
    return {};
}

Ref<Type> Type::common(const Value& val1, Ref<Type> type, const Value& val2) {
    return common(type);
}

// is_castable: type, val

bool Type::is_expl_castable_to(Ref<const Type> type, const Value& val) const {
    return is_castable_to(type, val, true);
}

bool Type::is_impl_castable_to(Ref<const Type> type, const Value& val) const {
    return is_castable_to(type, val, false);
}

bool Type::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    return false;
}

// is_castable: builtin type ID, val

bool Type::is_expl_castable_to(
    BuiltinTypeId bi_type_id, const Value& val) const {
    return is_castable_to(bi_type_id, val, true);
}

bool Type::is_impl_castable_to(
    BuiltinTypeId bi_type_id, const Value& val) const {
    return is_castable_to(bi_type_id, val, false);
}

bool Type::is_castable_to(
    BuiltinTypeId bi_type_id, const Value& val, bool expl) const {
    return false;
}

// is_castable: type

bool Type::is_expl_castable_to(Ref<const Type> type) const {
    return is_castable_to(type, true);
}

bool Type::is_impl_castable_to(Ref<const Type> type) const {
    return is_castable_to(type, false);
}

bool Type::is_castable_to(Ref<const Type> type, bool expl) const {
    return is_castable_to(type, Value{RValue{}}, expl);
}

// is_castable: builtin type ID

bool Type::is_expl_castable_to(BuiltinTypeId bi_type_id) const {
    return is_castable_to(bi_type_id, true);
}

bool Type::is_impl_castable_to(BuiltinTypeId bi_type_id) const {
    return is_castable_to(bi_type_id, false);
}

bool Type::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    return is_castable_to(bi_type_id, Value{RValue{}}, expl);
}

bool Type::is_expl_refable_as(Ref<const Type> type, const Value& val) const {
    return is_refable_as(type, val, true);
}

bool Type::is_impl_refable_as(Ref<const Type> type, const Value& val) const {
    return is_refable_as(type, val, false);
}

bool Type::is_refable_as(
    Ref<const Type> type, const Value& val, bool expl) const {
    return is_same(type);
}

bool Type::is_assignable_to(Ref<const Type> type, const Value& val) const {
    return is_same(type);
}

conv_cost_t Type::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return is_same(type) ? 0 : MaxConvCost;
}

conv_cost_t
Type::conv_cost(Ref<const Type> type, const Value& val, bool allow_cast) const {
    return conv_cost(type, allow_cast);
}

Value Type::cast_to(Ref<Type> type, Value&& val) { assert(false); }

Ptr<ArrayType> Type::make_array_type(array_size_t size) {
    return make<ArrayType>(_builtins, _id_gen, this, size);
}

Ptr<RefType> Type::make_ref_type() {
    return make<RefType>(_builtins, _id_gen, this);
}

// AliasType

const std::string_view AliasType::name() const {
    assert(_canon);
    return _str_pool.get(name_id());
}

str_id_t AliasType::name_id() const { return _node->alias_id(); }

bitsize_t AliasType::bitsize() const {
    assert(_canon);
    return _canon->bitsize();
}

bool AliasType::is_constructible() const {
    return non_alias()->is_constructible();
}

RValue AliasType::construct() { return non_alias()->construct(); }

RValue AliasType::load(const BitsView data, bitsize_t off) {
    return non_alias()->load(data, off);
}

void AliasType::store(BitsView data, bitsize_t off, const RValue& rval) {
    non_alias()->store(data, off, std::move(rval));
}

BuiltinTypeId AliasType::bi_type_id() const {
    return non_alias()->bi_type_id();
}

bool AliasType::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    return non_alias()->is_castable_to(type, val, expl);
}

bool AliasType::is_castable_to(
    BuiltinTypeId bi_type_id, const Value& val, bool expl) const {
    return non_alias()->is_castable_to(bi_type_id, val, expl);
}

bool AliasType::is_refable_as(
    Ref<const Type> type, const Value& val, bool expl) const {
    return non_alias()->is_refable_as(type, val, expl);
}

conv_cost_t AliasType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return non_alias()->conv_cost(type->non_alias(), allow_cast);
}

conv_cost_t AliasType::conv_cost(
    Ref<const Type> type, const Value& val, bool allow_cast) const {
    return non_alias()->conv_cost(type->non_alias(), val, allow_cast);
}

Value AliasType::cast_to(Ref<Type> type, Value&& val) {
    return non_alias()->cast_to(type, std::move(val));
}

void AliasType::set_aliased(Ref<Type> type) {
    assert(type);
    assert(!_aliased);
    _aliased = type;
    _canon = type->canon();
    _non_alias = type->non_alias();
}

Ref<ast::TypeName> AliasType::type_name() {
    assert(_node->type_name());
    return _node->type_name();
}

Ref<ast::TypeExpr> AliasType::type_expr() {
    assert(_node->type_expr());
    return _node->type_expr();
}

Ref<Type> AliasType::deref() { return non_alias()->deref(); }
Ref<const Type> AliasType::deref() const { return non_alias()->deref(); }

Ref<PrimType> AliasType::_as_prim() { return non_alias()->_as_prim(); }
Ref<const PrimType> AliasType::_as_prim() const {
    return non_alias()->_as_prim();
}

Ref<Class> AliasType::_as_class() { return non_alias()->_as_class(); }
Ref<const Class> AliasType::_as_class() const {
    return non_alias()->_as_class();
}

Ref<ArrayType> AliasType::_as_array() { return non_alias()->_as_array(); }
Ref<const ArrayType> AliasType::_as_array() const {
    return non_alias()->_as_array();
}

Ref<RefType> AliasType::_as_ref() { return non_alias()->_as_ref(); }
Ref<const RefType> AliasType::_as_ref() const { return non_alias()->_as_ref(); }

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

const std::string_view ArrayType::name() const {
    assert(_item_type);
    if (_name.empty()) {
        std::string size_str;
        if (_array_size != UnknownArraySize)
            size_str = std::to_string(_array_size);
        _name = std::string{_item_type->name()} + "[" + size_str + "]";
    }
    return _name;
}

bitsize_t ArrayType::bitsize() const {
    assert(_item_type);
    return _array_size * _item_type->bitsize();
}

RValue ArrayType::construct() {
    auto data = make_s<Data>(this);
    RValue rval;
    if (array_size() > 0)
        rval = item_type()->construct();
    bitsize_t off = 0;
    for (array_idx_t idx = 0; idx < array_size(); ++idx) {
        item_type()->store(data->bits(), off, rval);
        off += item_type()->bitsize();
    }
    return RValue{data, true};
}

RValue ArrayType::load(const BitsView data, bitsize_t off) {
    auto rval = construct();
    if (bitsize() > 0)
        rval.data_view().bits().write(0, data.view(off, bitsize()));
    return rval;
}

void ArrayType::store(BitsView data, bitsize_t off, const RValue& rval) {
    assert(rval.is<DataPtr>());
    if (bitsize() > 0)
        data.write(off, rval.get<DataPtr>()->bits().view());
}

TypedValue ArrayType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::LengthOf: {
        auto type = builtins().unsigned_type();
        RValue rval{(Unsigned)array_size(), true};
        return {type, Value{std::move(rval)}};
    }
    default:
        return Type::type_op(op);
    }
}

bool ArrayType::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    assert(!is_same(type));
    if (!type->is_array())
        return false;

    auto array_type = type->as_array();
    if (array_type->array_size() != array_size())
        return false;

    assert(!array_type->item_type()->is_same(item_type()));
    return (item_type()->is_castable_to(array_type->item_type(), expl));
}

Value ArrayType::cast_to(Ref<Type> type, Value&& val) {
    assert(!is_same(type));
    assert(type->is_array());

    auto array_type = type->as_array();
    assert(array_type->array_size() == array_size());
    assert(!array_type->item_type()->is_same(item_type()));

    auto rval = type->construct();
    rval.set_is_consteval(val.is_consteval());
    for (array_idx_t i = 0; i < array_size(); ++i) {
        auto item_lval = rval.array_access(i, true);
        auto item_rval =
            item_type()
                ->cast_to(
                    array_type->item_type(), Value{val.array_access(i, true)})
                .move_rvalue();
        item_lval.assign(std::move(item_rval));
    }
    return Value{std::move(rval)};
}

void ArrayType::set_canon(Ref<ArrayType> canon) {
    assert(canon);
    assert(canon->item_type() == item_type()->canon());
    assert(canon->array_size() == array_size());
    _canon = canon;
}

Ref<Type> ArrayType::non_array() {
    if (!_non_array) {
        Ref<ArrayType> type = this;
        while (true) {
            auto item_type = type->item_type();
            assert(item_type);
            if (!item_type->is_array()) {
                _non_array = item_type;
                break;
            }
            type = item_type->as_array();
        }
    }
    return _non_array;
}

Ref<const Type> ArrayType::non_array() const {
    return const_cast<ArrayType*>(this)->non_array();
}

bitsize_t ArrayType::item_off(array_idx_t idx) const {
    assert(idx < _array_size);
    return item_type()->bitsize() * idx;
}

// RefType

const std::string_view RefType::name() const {
    if (_name.empty()) {
        assert(_refd);
        _name = std::string{_refd->name()} + "&";
    }
    return _name;
}

bitsize_t RefType::bitsize() const {
    assert(_refd);
    return _refd->bitsize();
}

void RefType::set_canon(Ref<RefType> canon) {
    assert(canon);
    assert(canon->refd() == refd()->canon());
    _canon = canon;
}

bool RefType::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    if (!type->is_ref())
        return refd()->is_castable_to(type);
    return refd()->is_refable_as(type->deref(), val, expl);
}

bool RefType::is_castable_to(
    BuiltinTypeId bi_type_id, const Value& val, bool expl) const {
    return refd()->is_castable_to(bi_type_id, expl);
}

bool RefType::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    return is_castable_to(bi_type_id, Value{LValue{}}, expl);
}

bool RefType::is_refable_as(
    Ref<const Type> type, const Value& val, bool expl) const {
    assert(false);
}

bool RefType::is_assignable_to(Ref<const Type> type, const Value& val) const {
    assert(false);
}

conv_cost_t RefType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return refd()->conv_cost(type->actual(), allow_cast); // ??
}

conv_cost_t RefType::conv_cost(
    Ref<const Type> type, const Value& val, bool allow_cast) const {
    return refd()->conv_cost(type->actual(), val, allow_cast); // ??
}

Value RefType::cast_to(Ref<Type> type, Value&& val) {
    assert(val.is_lvalue());
    if (!type->is_ref())
        return refd()->cast_to(type, std::move(val));
    type = type->deref();
    assert(refd()->is_expl_refable_as(type, val));
    if (refd()->is_impl_refable_as(type, val))
        return std::move(val);
    return Value{val.lvalue().as(type)};
}

Ptr<ArrayType> RefType::make_array_type(array_size_t size) { assert(false); }

Ptr<RefType> RefType::make_ref_type() { assert(false); }

} // namespace ulam

#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

// Type

Type::~Type() {}

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

Ptr<ArrayType> Type::make_array_type(array_size_t size) {
    return make<ArrayType>(_id_gen, this, size);
}

Ptr<RefType> Type::make_ref_type() { return make<RefType>(_id_gen, this); }

// AliasType

str_id_t AliasType::name_id() const { return _node->alias_id(); }

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

void ArrayType::set_canon(Ref<ArrayType> canon) {
    assert(canon);
    assert(canon->item_type() == item_type()->canon());
    assert(canon->array_size() == array_size());
    _canon = canon;
}

// RefType

void RefType::set_canon(Ref<RefType> canon) {
    assert(canon);
    assert(canon->refd() == refd()->canon());
    _canon = canon;
}

} // namespace ulam

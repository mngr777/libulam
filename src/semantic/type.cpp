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
    auto type = make<ArrayType>(_id_gen, this, size);
    auto type_ref = ref(type);
    _array_types[size] = std::move(type);
    return type_ref;
}

Ref<RefType> Type::ref_type() {
    if (!_ref_type)
        _ref_type = make<RefType>(_id_gen, this);
    return ref(_ref_type);
}

// AliasType

str_id_t AliasType::name_id() const { return _node->alias_id(); }

ast::Ref<ast::TypeName> AliasType::type_name() {
    assert(_node->type_name());
    return _node->type_name();
}

ast::Ref<ast::TypeExpr> AliasType::type_expr() {
    assert(_node->type_expr());
    return _node->type_expr();
}

} // namespace ulam

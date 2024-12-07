#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

// Type

Type::~Type() {}

Ref<ArrayType> Type::array(array_size_t size) {
    // auto it = _array_types.find(size);
    // if (it != _array_types.end())
    //     return ref(it->second);
    // auto array_type = make<ArrayType>();
    return {};
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

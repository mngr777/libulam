#include <libulam/ast/nodes/type.hpp>
#include <libulam/sema/helper/resolver.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/ast/nodes/params.hpp>

namespace ulam {

Ref<Type>
Resolver::resolve(ast::Ref<ast::TypeName> type_name, Ref<Scope> scope) {
    // auto type = resolve(type_name->first(), scope);
    return {};
}

Ref<Type>
Resolver::resolve(ast::Ref<ast::TypeSpec> type_spec, Ref<Scope> scope) {
    return {};
}

} // namespace ulam

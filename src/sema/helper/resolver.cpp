#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/sema/helper/param_eval.hpp>
#include <libulam/sema/helper/resolver.hpp>

namespace ulam::sema {

Ref<Type>
Resolver::resolve_type(ast::Ref<ast::TypeName> type_name, Ref<Scope> scope) {
    
    return {};
}

} // namespace ulam::sema

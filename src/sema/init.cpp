#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/ph_type.hpp>

namespace ulam::sema {

bool Init::do_visit(ast::Ref<ast::TypeDef> node) {
    auto alias_str_id = node->name().str_id();
    if (scope()->has(alias_str_id, true)) {
        // TODO: after types are resolved, report error if types don't match
        return false;
    }
    scope()->set_placeholder(alias_str_id);
    return true;
}

bool Init::do_visit(ast::Ref<ast::TypeSpec> node) {
    if (!node->is_builtin())
        return true;
    assert(module_def()->module());
    auto str_id = node->ident()->name().str_id();
    if (!scope()->has(str_id, Scope::Module))
        module_def()->module()->add_import(node);
    return false;
}

} // namespace ulam::sema

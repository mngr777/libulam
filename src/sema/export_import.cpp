#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/export_import.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/ph_type.hpp>
#include <utility>

namespace ulam::sema {

bool ExportImport::do_visit(ast::Ref<ast::TypeDef> node) {
    // auto type_spec = node->type()->first();
    auto alias_id = node->name().str_id();

    auto alias_type = make<PhType>(program()->next_type_id());
    scope()->set(alias_id, std::move(alias_type));

    return true;
}

bool ExportImport::do_visit(ast::Ref<ast::TypeSpec> node) {
    // TODO
    return true;
}

} // namespace ulam::sema

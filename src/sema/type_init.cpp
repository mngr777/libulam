#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/type_init.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/ph_type.hpp>
#include <utility>

namespace ulam::sema {

bool TypeInit::do_visit(ast::Ref<ast::TypeDef> node) {
    auto name_id = node->name().str_id();
    if (scope()->has(name_id, true)) {
        diag().emit(
            diag::Error, node->name().loc_id(), str(name_id).size(),
            "type already defined");
        return false; // do not search for aliased type then
    }
    auto type = make<PhType>(program()->next_type_id());
    scope()->set(name_id, std::move(type));
    return true;
}

bool TypeInit::do_visit(ast::Ref<ast::TypeSpec> node) {
    // TODO
    return true;
}

} // namespace ulam::sema

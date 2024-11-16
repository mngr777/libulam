#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/type_init.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

bool TypeInit::do_visit(ast::Ref<ast::TypeDef> type_def) {
    return true;
}

} // namespace ulam::sema

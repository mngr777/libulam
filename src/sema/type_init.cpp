#include <libulam/sema/type_init.hpp>

namespace ulam::sema {

bool TypeInit::do_visit(ast::Ref<ast::ModuleDef> module) { return true; }

bool TypeInit::do_visit(ast::Ref<ast::ClassDef> class_def) { return true; }

} // namespace ulam::sema

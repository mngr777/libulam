#include <libulam/sema/init_members.hpp>

namespace ulam::sema {

bool InitMembers::visit(ast::Ref<ast::VarDefList> node) {
    auto def = class_def();
    if (!def) {
        // TODO: add local constants
    } else if (def->type()) {
    } else if (def->type_tpl()) {
    } else {
        assert(false);
    }
    return {};
}

bool InitMembers::do_visit(ast::Ref<ast::FunDef> node) { return {}; }

} // namespace ulam::sema

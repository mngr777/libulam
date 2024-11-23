#include <libulam/sema/init_members.hpp>

namespace ulam::sema {

bool InitMembers::visit(ast::Ref<ast::VarDefList> node) {
    return {};
}

bool InitMembers::do_visit(ast::Ref<ast::FunDef> node) { return {}; }

} // namespace ulam::sema

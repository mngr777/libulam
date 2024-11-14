#include <libulam/sema.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

Visitor::InScope::InScope(Sema& sema): _sema{sema} { _sema.enter_scope(); }
Visitor::InScope::~InScope() { _sema.exit_scope(); }

bool Visitor::visit(ast::ModuleDef& module) {
    auto scoped = in_scope();
    return do_visit(module);
}

bool Visitor::visit(ast::ClassDefBody& class_body) {
    auto scoped = in_scope();
    return do_visit(class_body);
}

} // namespace ulam::sema

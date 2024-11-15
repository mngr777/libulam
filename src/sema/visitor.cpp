#include "libulam/ast/nodes/module.hpp"
#include <libulam/sema.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

RecVisitor::InScope::InScope(Sema& sema): _sema{sema} { _sema.enter_scope(); }
RecVisitor::InScope::~InScope() { _sema.exit_scope(); }

bool RecVisitor::visit(ast::Ref<ast::ModuleDef> module_def) {
    auto scoped = in_scope();
    _module_def = module_def;
    if (do_visit(module_def))
        traverse(module_def);
    return false;
}

bool RecVisitor::visit(ast::Ref<ast::ClassDef> class_def) {
    _class_def = class_def;
    do_visit(class_def);
    traverse(class_def);
    return false;
}

bool RecVisitor::visit(ast::Ref<ast::ClassDefBody> class_def_body) {
    auto scoped = in_scope();
    do_visit(class_def_body);
    traverse(class_def_body);
    return false;
}

Scope* RecVisitor::scope() { return _sema.scope(); }

Diag& RecVisitor::diag() { return _sema.diag(); }

} // namespace ulam::sema

#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

bool RecVisitor::visit(ast::Ref<ast::ModuleDef> node) {
    // make module
    if (!node->module()) {
        auto module = ulam::make<Module>(node);
        auto module_ref = ref(module);
        program()->add_module(std::move(module));
        node->set_module(module_ref);
    }
    _module_def = node;
    _sema.enter_scope(node->module()->scope());
    if (do_visit(node))
        traverse(node);
    _sema.exit_scope();
    _module_def = {};
    return false;
}

bool RecVisitor::visit(ast::Ref<ast::ClassDef> node) {
    // make class
    if (!node->type()) {
        // make
        auto name_id = node->name_id();
        auto type = ulam::make<Class>(program()->next_type_id(), node);
        auto type_ref = ref(type);
        // add to module scope
        if (scope()->has(name_id, Scope::Module)) {
            // TODO: already defined where?
            diag().emit(
                diag::Error, node->name_loc_id(), str(name_id).size(),
                "already defined");
            return false;
        }
        assert(scope()->is(Scope::Module));
        scope()->set(name_id, std::move(type));
        // set node attr
        node->set_type(type_ref);
    }
    _class_def = node;
    if (do_visit(node))
        traverse(node);
    _class_def = {};
    return false;
}

bool RecVisitor::visit(ast::Ref<ast::ClassDefBody> node) {
    assert(_class_def && _class_def->type());
    _sema.enter_scope(_class_def->type()->scope());
    if (do_visit(node))
        traverse(node);
    _sema.exit_scope();
    return false;
}

Diag& RecVisitor::diag() { return _sema.diag(); }

Ref<Program> RecVisitor::program() {
    assert(_ast->program());
    return _ast->program();
}

Ref<Scope> RecVisitor::scope() { return _sema.scope(); }

} // namespace ulam::sema

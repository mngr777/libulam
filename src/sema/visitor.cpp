#include "libulam/ast/node.hpp"
#include "libulam/ast/nodes/module.hpp"
#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/class.hpp>

// TODO: move object initialization to sema::Init

namespace ulam::sema {

void RecVisitor::analyze() { visit(_ast); }

bool RecVisitor::visit(ast::Ref<ast::Root> node) {
    assert(node->program());
    enter_scope(Scope::Program);
    for (auto& module : node->program()->modules())
        module->export_symbols(scope());
    if (do_visit(node))
        traverse(node);
    exit_scope();
    return {};
}


bool RecVisitor::visit(ast::Ref<ast::ModuleDef> node) {
    assert(node->module());
    _module_def = node;
    assert(node->module());
    enter_scope(Scope::Module);
    if (do_visit(node))
        traverse(node);
    exit_scope();
    _module_def = {};
    return {};
}

bool RecVisitor::visit(ast::Ref<ast::ClassDef> node) {
    _class_def = node;
    if (do_visit(node))
        traverse(node);
    _class_def = {};
    return {};
}

bool RecVisitor::visit(ast::Ref<ast::ClassDefBody> node) {
    assert(_class_def && _class_def->type());
    enter_scope(Scope::Class);
    // TODO: export?
    if (do_visit(node))
        traverse(node);
    exit_scope();
    return {};
}

void RecVisitor::traverse(ast::Ref<ast::ClassDefBody> node) {
    traverse_class_defs(node);
    if (!_skip_fun_bodies)
        traverse_fun_bodies(node);
}

bool RecVisitor::visit(ast::Ref<ast::FunDefBody> node) {
    assert(_fun_def);
    enter_scope(Scope::Fun);
    // NOTE: params are not added to scope here TODO?
    if (do_visit(node))
        traverse(node);
    exit_scope();
    return {};
}

void RecVisitor::traverse_class_defs(ast::Ref<ast::ClassDefBody> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (ast::is<ast::FunDef>(child_v)) {
            // directly visit fun def
            do_visit(ast::as_ref<ast::FunDef>(child_v));
        } else {
            // continue as usual
            ast::as_node_ref(child_v)->accept(*this);
        }
    }
}

void RecVisitor::traverse_fun_bodies(ast::Ref<ast::ClassDefBody> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (ast::is<ast::FunDef>(child_v))
            traverse(ast::as_ref<ast::FunDef>(child_v));
    }
}

void RecVisitor::traverse(ast::Ref<ast::FunDef> node) {
    _fun_def = node;
    enter_scope(Scope::Fun);
    // TODO: add params to scope
    node->body()->accept(*this);
    exit_scope();
    _fun_def = {};
}


void RecVisitor::enter_scope(Scope::Flag flags) {
    auto parent = _scopes.size() ? &_scopes.top() : nullptr;
    _scopes.emplace(parent, flags);
}

void RecVisitor::exit_scope() {
    _scopes.pop();
}

Scope* RecVisitor::scope() {
    assert(!_scopes.empty());
    return &_scopes.top();
}

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

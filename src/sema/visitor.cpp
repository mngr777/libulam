#include "libulam/semantic/scope.hpp"
#include "libulam/semantic/scope/version.hpp"
#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/class.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::RecVisitor] "
#include "src/debug.hpp"

namespace ulam::sema {

void RecVisitor::analyze() { visit(_ast); }

void RecVisitor::visit(Ref<ast::Root> node) {
    assert(node->program());
    if (do_visit(node))
        traverse(node);
}

void RecVisitor::visit(Ref<ast::ModuleDef> node) {
    assert(node->module());
    auto mod = node->module();
    _module_def = node;
    enter_module_scope(mod);
    if (do_visit(node)) {
        traverse(node);
        _pass = Pass::Classes;
        traverse(node);
        if (!_skip_fun_bodies) {
            _pass = Pass::FunBodies;
            traverse(node);
        }
    }
    assert(scope()->is(scp::Module));
    exit_scope();
    _module_def = {};
}

void RecVisitor::traverse(Ref<ast::ModuleDef> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Module || ast::is<ast::ClassDef>(child_v))
            ast::as_node_ref(child_v)->accept(*this);
    }
}

void RecVisitor::visit(Ref<ast::ClassDef> node) {
    if (pass() == Pass::Module) {
        do_visit(node);
    } else {
        _class_def = node;
        traverse(node);
        _class_def = {};
    }
}

void RecVisitor::traverse(Ref<ast::ClassDefBody> node) {
    assert(pass() == Pass::Classes || pass() == Pass::FunBodies);
    assert(_class_def);
    // enter scope
    if (_class_def->type()) {
        enter_class_scope(_class_def->type());
    } else {
        assert(_class_def->type_tpl());
        enter_class_tpl_scope(_class_def->type_tpl());
    }
    // traverse
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Classes || ast::is<ast::FunDef>(child_v))
            ast::as_node_ref(child_v)->accept(*this);
    }
    // exit scope
    assert(scope()->is(scp::Class) || scope()->is(scp::ClassTpl));
    exit_scope();
}

void RecVisitor::visit(Ref<ast::FunDef> node) {
    if (pass() == Pass::Classes) {
        do_visit(node);
    } else {
        assert(pass() == Pass::FunBodies);
        _fun_def = node;
        traverse(node);
        _fun_def = {};
    }
}

void RecVisitor::traverse(Ref<ast::FunDefBody> node) {
    assert(pass() == Pass::FunBodies);
    assert(_fun_def);
    enter_scope(scp::Fun);
    ast::RecVisitor::traverse(node);
    assert(scope()->is(scp::Fun));
    exit_scope();
}

void RecVisitor::visit(Ref<ast::Block> node) {
    enter_scope();
    ast::RecVisitor::visit(node);
    exit_scope();
}

bool RecVisitor::do_visit(Ref<ast::ClassDef> node) {
    assert(scope()->is(scp::Module));
    _scopes.top<PersScopeProxy>()->set_version_after(node->scope_version());
    return true;
}

bool RecVisitor::do_visit(Ref<ast::TypeDef> node) {
    if (_scopes.top_is<PersScopeProxy>()) {
        _scopes.top<PersScopeProxy>()->set_version_after(node->scope_version());
    } else {
        // TODO: transient typedef
    }
    return true;
}

bool RecVisitor::do_visit(Ref<ast::VarDef> node) {
    if (_scopes.top_is<PersScopeProxy>()) {
        _scopes.top<PersScopeProxy>()->set_version_after(node->scope_version());
    } else {
        // TODO: transient var decl
    }
    return true;
}

bool RecVisitor::do_visit(Ref<ast::FunDef> node) {
    _scopes.top<PersScopeProxy>()->set_version_after(node->scope_version());
    return true;
}

void RecVisitor::enter_module_scope(Ref<Module> mod) {
    assert(pass() == Pass::Module);
    auto scope = mod->scope()->proxy();
    scope.reset();
    enter_scope(std::move(scope));
}

void RecVisitor::enter_class_scope(Ref<Class> cls) {
    auto scope = cls->scope()->proxy();
    if (pass() == Pass::Classes)
        scope.reset();
    enter_scope(std::move(scope));
}

void RecVisitor::enter_class_tpl_scope(Ref<ClassTpl> tpl) {
    auto scope = tpl->scope()->proxy();
    if (pass() == Pass::Classes)
        scope.reset();
    enter_scope(std::move(scope));
}

void RecVisitor::enter_scope(ScopeFlags flags) {
    auto parent = !_scopes.empty() ? _scopes.top<Scope>() : Ref<Scope>{};
    _scopes.push(make<BasicScope>(parent, flags));
}

void RecVisitor::enter_scope(PersScopeProxy&& scope) {
    _scopes.push(std::move(scope));
}

void RecVisitor::exit_scope() { _scopes.pop(); }

Scope* RecVisitor::scope() {
    assert(!_scopes.empty());
    return _scopes.top<Scope>();
}

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

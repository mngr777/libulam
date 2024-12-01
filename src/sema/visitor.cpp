#include "libulam/ast/visitor.hpp"
#include "libulam/ast/node.hpp"
#include "libulam/ast/nodes/module.hpp"
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

bool RecVisitor::visit(ast::Ref<ast::Root> node) {
    assert(node->program());
    enter_scope(program()->scope());
    if (do_visit(node))
        traverse(node);
    exit_scope();
    return {};
}

bool RecVisitor::visit(ast::Ref<ast::ModuleDef> node) {
    assert(node->module());
    auto mod = node->module();
    _module_def = node;
    enter_module_scope(mod);
    mod->export_imports(scope());
    if (do_visit(node)) {
        _pass = Pass::Module;
        traverse(node);
        _pass = Pass::Classes;
        traverse(node);
        if (!_skip_fun_bodies) {
            _pass = Pass::FunBodies;
            traverse(node);
        }
    }
    assert(scope()->is(Scope::Module));
    exit_scope();
    _module_def = {};
    return {};
}

void RecVisitor::traverse(ast::Ref<ast::ModuleDef> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Module || ast::is<ast::ClassDef>(child_v))
            ast::as_node_ref(child_v)->accept(*this);
    }
}

bool RecVisitor::visit(ast::Ref<ast::ClassDef> node) {
    if (pass() == Pass::Module) {
        do_visit(node);
    } else {
        _class_def = node;
        traverse(node);
        _class_def = {};
    }
    return {};
}

void RecVisitor::traverse(ast::Ref<ast::ClassDefBody> node) {
    assert(pass() == Pass::Classes || pass() == Pass::FunBodies);
    assert(_class_def);
    // enter scope
    if (_class_def->type()) {
        enter_class_scope(_class_def->type());
    } else {
        assert(_class_def->type_tpl());
        enter_tpl_scope(_class_def->type_tpl());
    }
    // traverse
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Classes || ast::is<ast::FunDef>(child_v))
            ast::as_node_ref(child_v)->accept(*this);
    }
    // exit scope
    assert(scope()->is(Scope::Class) || scope()->is(Scope::ClassTpl));
    exit_scope();
}

bool RecVisitor::visit(ast::Ref<ast::FunDef> node) {
    if (pass() == Pass::Classes) {
        do_visit(node);
    } else {
        assert(pass() == Pass::FunBodies);
        _fun_def = node;
        traverse(node);
        _fun_def = {};
    }
    return {};
}

void RecVisitor::traverse(ast::Ref<ast::FunDefBody> node) {
    assert(pass() == Pass::FunBodies);
    assert(_fun_def);
    enter_scope(Scope::Fun);
    ast::RecVisitor::traverse(node);
    assert(scope()->is(Scope::Fun));
    exit_scope();
}

bool RecVisitor::visit(ast::Ref<ast::Block> node) {
    enter_scope();
    ast::RecVisitor::visit(node);
    exit_scope();
    return {};
}

bool RecVisitor::do_visit(ast::Ref<ast::ClassDef> node) {
    assert(scope()->is(Scope::Module));
    auto name_id = node->name().str_id();
    if (node->type()) {
        scope()->set(name_id, node->type());
    } else if (node->type_tpl()) {
        scope()->set(name_id, node->type_tpl());
    }
    return true;
}

bool RecVisitor::do_visit(ast::Ref<ast::TypeDef> node) {
    auto name_id = node->name().str_id();
    Ref<Type> type{node->alias_type()};
    if (type)
        scope()->set(name_id, type);
    return true;
}

bool RecVisitor::do_visit(ast::Ref<ast::VarDef> node) {
    auto name_id = node->name().str_id();
    auto var = node->var();
    if (var)
        scope()->set(name_id, var);
    return true;
}

bool RecVisitor::do_visit(ast::Ref<ast::FunDef> node) {
    auto name_id = node->name().str_id();
    auto overload = node->overload();
    auto sym = scope()->get(name_id);
    if (sym) {
        if (!sym->is<Fun>())
            return false;
    } else {
        sym = scope()->set(name_id, ulam::make<Fun>());
    }
    sym->get<Fun>()->add_overload(overload);
    return true;
}

void RecVisitor::enter_module_scope(Ref<Module> module) {
    if (pass() == Pass::Module) {
        enter_scope(make<Scope>(module, Ref<Scope>{}, Scope::Module));
    } else {
        enter_scope(module->scope());
    }
}

void RecVisitor::enter_class_scope(Ref<Class> cls) {
    if (pass() == Pass::Classes) {
        enter_scope(Scope::Class);
    } else {
        enter_scope(cls->scope());
    }
}

void RecVisitor::enter_tpl_scope(Ref<ClassTpl> tpl) {
    if (pass() == Pass::Classes) {
        enter_scope(Scope::Class);
    } else {
        enter_scope(tpl->scope());
    }
}

void RecVisitor::enter_scope(Scope::Flag flags) {
    auto parent = _scopes.size() ? _scopes.top().ref : Ref<Scope>{};
    _scopes.emplace(make<Scope>(parent, flags));
}

void RecVisitor::enter_scope(Ptr<Scope>&& scope) {
    _scopes.emplace(std::move(scope));
}

void RecVisitor::enter_scope(Ref<Scope> scope) { _scopes.emplace(scope); }

void RecVisitor::exit_scope() { _scopes.pop(); }

Scope* RecVisitor::scope() {
    assert(!_scopes.empty());
    return _scopes.top().ref;
}

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

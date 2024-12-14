#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/view.hpp>
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
    if (_class_def->cls()) {
        enter_class_scope(_class_def->cls());
    } else {
        assert(_class_def->cls_tpl());
        enter_class_tpl_scope(_class_def->cls_tpl());
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
    if (do_visit(node))
        traverse(node);
    assert(scope()->is(scp::Fun));
    exit_scope();
}

void RecVisitor::visit(Ref<ast::Block> node) {
    enter_scope();
    if (do_visit(node))
        traverse(node);
    exit_scope();
}

void RecVisitor::visit(Ref<ast::For> node) {
    enter_scope(scp::Break | scp::Continue);
    if (do_visit(node))
        traverse(node);
    exit_scope();
}

void RecVisitor::visit(Ref<ast::While> node) {
    enter_scope(scp::Break | scp::Continue);
    if (do_visit(node))
        traverse(node);
    exit_scope();
}

bool RecVisitor::do_visit(Ref<ast::ClassDef> node) {
    assert(scope()->is(scp::Module));
    auto synced = sync_scope(node);
    assert(synced);
    return true;
}

bool RecVisitor::do_visit(Ref<ast::TypeDef> node) {
    if (!sync_scope(node)) {
        Resolver resolver{program()};
        Ptr<UserType> type = make<AliasType>(nullptr, node);
        resolver.resolve(type->as_alias(), scope());
        scope()->set(type->name_id(), std::move(type));
    }
    return true;
}

void RecVisitor::visit(Ref<ast::VarDefList> node) {
    Resolver resolver{program()};
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto def = node->def(n);
        if (!sync_scope(def)) {
            // local variable
            Var::Flag flags = Var::NoFlags;
            if (node->is_const())
                flags |= Var::Const;
            auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
            if (resolver.resolve(ref(var), scope())) {
                if (def->has_default_value()) {
                    ExprVisitor ev{ast(), scope()};
                    ExprRes res = def->default_value()->accept(ev);
                    if (res.ok()) {
                        // TODO: conversion/type error
                        auto tv = res.move_typed_value();
                        var->value() = tv.move_value();
                    }
                }
                scope()->set(var->name_id(), std::move(var));
            }
        }
    }
}

bool RecVisitor::do_visit(Ref<ast::FunDef> node) {
    auto synced = sync_scope(node);
    assert(synced);
    return true;
}

bool RecVisitor::sync_scope(Ref<ast::DefNode> node) {
    auto scope = _scopes.top();
    assert(scope->has_version() == (node->scope_version() != NoScopeVersion));
    if (scope->has_version()) {
        scope->set_version_after(node->scope_version());
        return true;
    }
    return false;
}

void RecVisitor::enter_module_scope(Ref<Module> mod) {
    assert(pass() == Pass::Module);
    enter_scope(mod->scope()->view(0));
}

void RecVisitor::enter_class_scope(Ref<Class> cls) {
    auto scope = cls->scope()->view();
    if (pass() == Pass::Classes)
        scope->reset();
    enter_scope(std::move(scope));
}

void RecVisitor::enter_class_tpl_scope(Ref<ClassTpl> tpl) {
    auto scope = tpl->scope()->view();
    if (pass() == Pass::Classes)
        scope->reset();
    enter_scope(std::move(scope));
}

void RecVisitor::enter_scope(ScopeFlags flags) { _scopes.push(flags); }

void RecVisitor::enter_scope(Ptr<Scope>&& scope) {
    _scopes.push(std::move(scope));
}

void RecVisitor::exit_scope() { _scopes.pop(); }

Scope* RecVisitor::scope() { return _scopes.top(); }

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

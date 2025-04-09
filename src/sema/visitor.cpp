#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam::sema {

RecVisitor::RecVisitor(Diag& diag, Ref<ast::Root> ast, bool skip_fun_bodies):
    _diag{diag},
    _ast{ast},
    _program{},
    _eval{},
    _skip_fun_bodies{skip_fun_bodies},
    _pass{Pass::Module} {}

RecVisitor::~RecVisitor() {}

void RecVisitor::analyze() { visit(_ast); }

void RecVisitor::visit(Ref<ast::Root> node) {
    assert(node->program());
    _program = node->program();
    _eval = make<EvalVisitor>(_program);

    if (do_visit(node))
        traverse(node);
}

void RecVisitor::visit(Ref<ast::ModuleDef> node) {
    assert(node->module());
    auto mod = node->module();
    _module_def = node;
    enter_module_scope(mod);
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
    assert(scope()->is(scp::Module));
    exit_scope();
    _module_def = {};
}

void RecVisitor::traverse(Ref<ast::ModuleDef> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Module || child_v.is<Ptr<ast::ClassDef>>())
            child_v.accept([&](auto&& ptr) { ptr->accept(*this); });
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
        if (pass() == Pass::Classes || child_v.is<Ptr<ast::FunDef>>())
            child_v.accept([&](auto&& ptr) { ptr->accept(*this); });
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

void RecVisitor::visit(Ref<ast::FunDefBody> node) {
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
        Ptr<UserType> type = make<AliasType>(
            program()->str_pool(), program()->builtins(), nullptr, node);
        _eval->resolver()->resolve(type->as_alias(), scope());
        scope()->set(type->name_id(), std::move(type));
    }
    return true;
}

void RecVisitor::visit(Ref<ast::VarDefList> node) {
    auto resolver = _eval->resolver();
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto def = node->def(n);
        if (!sync_scope(def)) {
            // local variable
            Var::Flag flags = Var::NoFlags;
            if (node->is_const())
                flags |= Var::Const;
            auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
            if (resolver->resolve(ref(var), scope())) {
                if (!var->has_value() && def->has_init()) {
                    auto init = _eval->init_helper(scope());
                    auto [val, ok] = init->eval(var->type(), def->init());
                    if (ok)
                        var->set_value(std::move(val));
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
        scope->set_version(node->scope_version());
        return true;
    }
    return false;
}

void RecVisitor::enter_module_scope(Ref<Module> mod) {
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

void RecVisitor::enter_scope(scope_flags_t flags) { _scopes.push(flags); }

void RecVisitor::enter_scope(Ptr<Scope>&& scope) {
    _scopes.push(std::move(scope));
}

void RecVisitor::exit_scope() { _scopes.pop(); }

Scope* RecVisitor::scope() { return _scopes.top(); }

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

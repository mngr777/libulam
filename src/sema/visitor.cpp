#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam::sema {

RecVisitor::RecVisitor(Diag& diag, Ref<ast::Root> ast, bool skip_fun_bodies):
    _diag{diag},
    _ast{ast},
    _program{},
    _eval_env{},
    _skip_fun_bodies{skip_fun_bodies},
    _pass{Pass::Module} {}

RecVisitor::~RecVisitor() {}

void RecVisitor::analyze() { visit(_ast); }

void RecVisitor::visit(Ref<ast::Root> node) {
    assert(node->program());
    _program = node->program();
    _eval_env = make<EvalEnv>(_program); // ??

    if (do_visit(node))
        traverse(node);
}

void RecVisitor::visit(Ref<ast::ModuleDef> node) {
    assert(node->module());
    auto mod = node->module();
    _module_def = node;
    auto scope_raii =
        _scope_stack.raii<PersScopeView>(mod->scope(), ScopeVersion{0});
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
    assert((bool)_class_def->cls() != (bool)_class_def->cls_tpl());

    auto scope_version = (pass() == Pass::Classes) ? 0 : NoScopeVersion;
    auto scope_raii = _scope_stack.raii<PersScopeView>(
        _class_def->cls_or_tpl()->scope(), scope_version);

    // traverse
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (pass() == Pass::Classes || child_v.is<Ptr<ast::FunDef>>())
            child_v.accept([&](auto&& ptr) { ptr->accept(*this); });
    }
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
    auto scope_raii = _scope_stack.raii<BasicScope>(scope(), scp::Fun);
    if (do_visit(node))
        traverse(node);
    assert(scope()->is(scp::Fun));
}

void RecVisitor::visit(Ref<ast::Block> node) {
    auto scope_raii = _scope_stack.raii<BasicScope>(scope());
    if (do_visit(node))
        traverse(node);
}

void RecVisitor::visit(Ref<ast::For> node) {
    auto scope_raii =
        _scope_stack.raii<BasicScope>(scope(), scp::BreakAndContinue);
    if (do_visit(node))
        traverse(node);
}

void RecVisitor::visit(Ref<ast::While> node) {
    auto scope_raii =
        _scope_stack.raii<BasicScope>(scope(), scp::BreakAndContinue);
    if (do_visit(node))
        traverse(node);
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
        _eval_env->resolver(false).resolve(type->as_alias());
        scope()->set(type->name_id(), std::move(type));
    }
    return true;
}

void RecVisitor::visit(Ref<ast::VarDefList> node) {
    auto resolver = _eval_env->resolver(false);
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto def = node->def(n);
        if (!sync_scope(def)) {
            // local variable
            Var::flags_t flags = Var::NoFlags;
            if (node->is_const())
                flags |= Var::Const;
            auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
            if (resolver.resolve(ref(var)))
                scope()->set(var->name_id(), std::move(var));
        }
    }
}

bool RecVisitor::do_visit(Ref<ast::FunDef> node) {
    auto synced = sync_scope(node);
    assert(synced);
    return true;
}

bool RecVisitor::sync_scope(Ref<ast::DefNode> node) {
    auto scope_v = _scope_stack.top_v();
    return scope_v.accept(
        [&](PersScopeView* scope_view) {
            scope_view->set_version(node->scope_version());
            return true;
        },
        [&](auto) { return false; });
}

Scope* RecVisitor::scope() { return _scope_stack.top(); }

Diag& RecVisitor::diag() { return _diag; }

} // namespace ulam::sema

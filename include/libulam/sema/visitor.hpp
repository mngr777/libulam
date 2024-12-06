#pragma once
#include <cassert>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/str_pool.hpp>
#include <stack>

namespace ulam {
class Program;
} // namespace ulam

namespace ulam::sema {

class RecVisitor : public ast::RecVisitor {
public:
    using ast::RecVisitor::do_visit;
    using ast::RecVisitor::traverse;
    using ast::RecVisitor::visit;

    enum class Pass { Module, Classes, FunBodies };

    RecVisitor(
        Diag& diag, ast::Ref<ast::Root> ast, bool skip_fun_bodies = false):
        _diag{diag}, _ast{ast}, _skip_fun_bodies{skip_fun_bodies}, _pass{Pass::Module} {}

    void analyze();

protected:

    // Traversing in module defs / class defs / function body order
    void visit(ast::Ref<ast::Root> node) override;
    void visit(ast::Ref<ast::ModuleDef> node) override;
    void traverse(ast::Ref<ast::ModuleDef> node) override;
    void visit(ast::Ref<ast::ClassDef> node) override;
    void traverse(ast::Ref<ast::ClassDefBody> node) override;
    void visit(ast::Ref<ast::FunDef> node) override;
    void traverse(ast::Ref<ast::FunDefBody> node) override;

    // Creating transient scopes (TODO)
    void visit(ast::Ref<ast::Block> node) override;

    // Populating current scope with objects from AST
    bool do_visit(ast::Ref<ast::ClassDef> node) override;
    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::VarDef> node) override;
    bool do_visit(ast::Ref<ast::FunDef> node) override;

    // Handling persistent scopes
    void enter_module_scope(Ref<Module> module);
    void enter_class_scope(Ref<Class> cls);
    void enter_class_tpl_scope(Ref<ClassTpl> tpl);

    void enter_scope(PersScopeProxy&& scope);
    void enter_scope(Scope::Flag flags = Scope::NoFlags);
    void exit_scope();

    Diag& diag();
    ScopeStack& scopes() { return _scopes; }
    Ref<Scope> scope();

    Pass pass() { return _pass; }

    Ref<Program> program() {
        assert(ast()->program());
        return ast()->program();
    }

    ast::Ref<ast::Root> ast() {
        assert(_ast);
        return _ast;
    }

    Ref<Module> module() {
        assert(module_def());
        assert(module_def()->module());
        return module_def()->module();
    }

    ast::Ref<ast::ModuleDef> module_def() { return _module_def; }
    ast::Ref<ast::ClassDef> class_def() { return _class_def; }
    ast::Ref<ast::FunDef> fun_def() { return _fun_def; }

    const std::string_view str(str_id_t str_id) {
        return _ast->ctx().str(str_id);
    }

private:
    Diag& _diag;
    ast::Ref<ast::Root> _ast;
    bool _skip_fun_bodies;

    Pass _pass;

    ast::Ref<ast::ModuleDef> _module_def{};
    ast::Ref<ast::ClassDef> _class_def{};
    ast::Ref<ast::FunDef> _fun_def{};

    ScopeStack _scopes;
};

} // namespace ulam::sema

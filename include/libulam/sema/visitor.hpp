#pragma once
#include <cassert>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {
class Program;
} // namespace ulam

namespace ulam::sema {

class EvalVisitor;

class RecVisitor : public ast::RecVisitor {
public:
    using ast::RecVisitor::do_visit;
    using ast::RecVisitor::traverse;
    using ast::RecVisitor::visit;

    enum class Pass { Module, Classes, FunBodies };

    RecVisitor(Diag& diag, Ref<ast::Root> ast, bool skip_fun_bodies = false);
    ~RecVisitor();

    void analyze();

protected:
    // Traversing in module defs / class defs / function body order
    void visit(Ref<ast::Root> node) override;
    void visit(Ref<ast::ModuleDef> node) override;
    void traverse(Ref<ast::ModuleDef> node) override;
    void visit(Ref<ast::ClassDef> node) override;
    void traverse(Ref<ast::ClassDefBody> node) override;
    void visit(Ref<ast::FunDef> node) override;
    void visit(Ref<ast::FunDefBody> node) override;

    // Creating transient scopes (TODO)
    void visit(Ref<ast::Block> node) override;
    void visit(Ref<ast::For> node) override;
    void visit(Ref<ast::While> node) override;

    // Populating current scope with objects from AST
    bool do_visit(Ref<ast::ClassDef> node) override;
    bool do_visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;
    bool do_visit(Ref<ast::FunDef> node) override;

    bool sync_scope(Ref<ast::DefNode> node);

    // Handling persistent scopes
    void enter_module_scope(Ref<Module> module);
    void enter_class_scope(Ref<Class> cls);
    void enter_class_tpl_scope(Ref<ClassTpl> tpl);

    void enter_scope(Ptr<Scope>&& scope);
    void enter_scope(ScopeFlags flags = scp::NoFlags);
    void exit_scope();

    Diag& diag();
    ScopeStack& scopes() { return _scopes; }
    Ref<Scope> scope();

    Pass pass() { return _pass; }

    Ref<ast::Root> ast() {
        assert(_ast);
        return _ast;
    }

    Ref<Program> program() {
        assert(_program);
        return _program;
    }


    Ref<Module> module() {
        assert(module_def());
        assert(module_def()->module());
        return module_def()->module();
    }

    Ref<ast::ModuleDef> module_def() { return _module_def; }
    Ref<ast::ClassDef> class_def() { return _class_def; }
    Ref<ast::FunDef> fun_def() { return _fun_def; }

    const std::string_view str(str_id_t str_id) {
        return _ast->ctx().str(str_id);
    }

private:
    Diag& _diag;
    Ref<ast::Root> _ast;
    Ref<Program> _program;
    Ptr<EvalVisitor> _eval;
    bool _skip_fun_bodies;

    Pass _pass;

    Ref<ast::ModuleDef> _module_def{};
    Ref<ast::ClassDef> _class_def{};
    Ref<ast::FunDef> _fun_def{};

    ScopeStack _scopes;
};

} // namespace ulam::sema

#pragma once
#include <cassert>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/str_pool.hpp>
#include <stack>
#include <utility>

namespace ulam {
class Diag;
class Program;
} // namespace ulam

namespace ulam::sema {

class RecVisitor : public ast::RecVisitor {
public:
    using ast::RecVisitor::traverse;
    using ast::RecVisitor::visit;

    RecVisitor(
        Diag& diag, ast::Ref<ast::Root> ast, bool skip_fun_bodies = false):
        _diag{diag}, _ast{ast}, _skip_fun_bodies{skip_fun_bodies} {}

    void analyze();

protected:
    bool visit(ast::Ref<ast::Root> node) override;
    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool visit(ast::Ref<ast::ClassDef> node) override;
    bool visit(ast::Ref<ast::ClassDefBody> node) override;
    void traverse(ast::Ref<ast::ClassDefBody> node) override;
    bool visit(ast::Ref<ast::FunDef> node) override { assert(false); }
    void traverse(ast::Ref<ast::FunDef> node) override;
    bool visit(ast::Ref<ast::FunDefBody> node) override;
    // TODO: blocks, loops, ...

    void traverse_class_defs(ast::Ref<ast::ClassDefBody> node);
    void traverse_fun_bodies(ast::Ref<ast::ClassDefBody> node);

    void enter_scope(Ref<Scope> scope);
    void enter_scope(Scope::Flag flags = Scope::NoFlags);
    void exit_scope();

    Diag& diag();
    Ref<Scope> scope();

    Ref<Program> program() {
        assert(_ast->program());
        return _ast->program();
    }
    ast::Ref<ast::Root> ast() {
        assert(_ast);
        return _ast;
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

    ast::Ref<ast::ModuleDef> _module_def{};
    ast::Ref<ast::ClassDef> _class_def{};
    ast::Ref<ast::FunDef> _fun_def{};

    std::stack<std::pair<Ref<Scope>, Ptr<Scope>>> _scopes;
};

} // namespace ulam::sema

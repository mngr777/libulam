#pragma once
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/visitor.hpp"
#include <libulam/ast/nodes.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/semantic/scope.hpp> // TODO: move to .hpp

namespace ulam {
class Diag;
class Program;
class Scope;
class Sema;
} // namespace ulam

namespace ulam::sema {

class RecVisitor : public ast::RecVisitor {
public:
    using ast::RecVisitor::visit;

    RecVisitor(Sema& sema, ast::Ref<ast::Root> ast): _sema{sema}, _ast{ast} {}

    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool visit(ast::Ref<ast::ClassDef> node) override;
    bool visit(ast::Ref<ast::ClassDefBody> node) override;
    // TODO: blocks, loops, ...

protected:
    Diag& diag();
    Ref<Program> program();
    Ref<Scope> scope();

    ast::Ref<ast::Root> ast() { return _ast; }
    ast::Ref<ast::ModuleDef> module_def() { return _module_def; }
    ast::Ref<ast::ClassDef> class_def() { return _class_def; }

    const std::string_view str(str_id_t str_id) {
        return _ast->ctx().str(str_id);
    }

    // TODO: private?

    Sema& _sema;
    ast::Ref<ast::Root> _ast;

    ast::Ref<ast::ModuleDef> _module_def{};
    ast::Ref<ast::ClassDef> _class_def{};
};

} // namespace ulam::sema

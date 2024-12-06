#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <list>

namespace ulam::sema {

class ResolveDeps : public RecVisitor {
    using RecVisitor::do_visit;
    using RecVisitor::enter_scope;
    using RecVisitor::visit;

public:
    ResolveDeps(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    void visit(ast::Ref<ast::Root> node) override;
    void visit(ast::Ref<ast::ModuleDef> node) override;
    bool do_visit(ast::Ref<ast::ClassDef> node) override;
    void visit(ast::Ref<ast::TypeDef> node) override;
    void visit(ast::Ref<ast::VarDefList> node) override;
    void visit(ast::Ref<ast::FunDef> node) override;

    bool do_visit(ast::Ref<ast::TypeName> node) override;

private:
    struct ModuleTypeName {
        module_id_t module_id;
        ast::Ref<ast::TypeName> node;
    };

    void export_classes();

    std::list<ModuleTypeName> _unresolved;
};

} // namespace ulam::sema

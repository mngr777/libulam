#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/module.hpp>

namespace ulam::sema {

class Init : public RecVisitor {
    using RecVisitor::do_visit;
    using RecVisitor::enter_scope;
    using RecVisitor::visit;

public:
    Init(Diag& diag, Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    void visit(Ref<ast::Root> node) override;
    void visit(Ref<ast::ModuleDef> node) override;
    bool do_visit(Ref<ast::ClassDef> node) override;
    void visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;
    bool do_visit(Ref<ast::FunDef> node) override;
    bool do_visit(Ref<ast::TypeName> node) override;

private:
    void export_classes();
};

} // namespace ulam::sema

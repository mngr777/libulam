#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/export_import.hpp>

namespace ulam::sema {

class ResolveDeps : public RecVisitor {
    using RecVisitor::do_visit;
    using RecVisitor::visit;

public:
    ResolveDeps(Diag& diag, ast::Ref<ast::Root> ast):
        RecVisitor{diag, ast, true /* skip fun bodies*/} {}

    bool visit(ast::Ref<ast::Root> node) override;
    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool visit(ast::Ref<ast::VarDefList> node) override;

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::FunDef> node) override;
    bool do_visit(ast::Ref<ast::TypeName> node) override;

private:
    void init_classes(Ref<Module> module);
    void init_class(Ref<Module> module, ast::Ref<ast::ClassDef> node);
};

} // namespace ulam::sema

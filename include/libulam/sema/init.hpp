#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/export_import.hpp>

namespace ulam::sema {

class Init : public sema::RecVisitor {
public:
    explicit Init(Diag& diag, ast::Ref<ast::Root> ast):
        RecVisitor{diag, ast} {}

    bool visit(ast::Ref<ast::Root> node) override;
    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool visit(ast::Ref<ast::ClassDef> node) override;

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::TypeSpec> node) override;
};

} // namespace ulam::sema

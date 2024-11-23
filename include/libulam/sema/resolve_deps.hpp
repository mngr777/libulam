#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/export_import.hpp>

namespace ulam::sema {

// Creates empty classes/class templates, checks inter-module dependencies.
// For each module:
// * before traversing the subtree, each class def is added to the module
// (see `init_classes`);
// * whole subtree is traversed and all unseen type/template names are added to
// module's list of imports. After all modules have been processed,
// `check_module_deps` checks for duplicates and unsatified dependencies.
// TODO: update
class ResolveDeps : public RecVisitor {
    using RecVisitor::visit;
    using RecVisitor::do_visit;
public:
    ResolveDeps(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    bool visit(ast::Ref<ast::Root> node) override;
    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool visit(ast::Ref<ast::VarDefList> node) override;

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::TypeName> node) override;

private:
    void init_classes(Ref<Module> module, ast::Ref<ast::ModuleDef> node);
    void init_class(Ref<Module> module, ast::Ref<ast::ClassDef> node);

    void check_module_deps();
};

} // namespace ulam::sema

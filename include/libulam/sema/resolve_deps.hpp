#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/export_import.hpp>
#include <libulam/semantic/module.hpp>
#include <list>

namespace ulam::sema {

class ResolveDeps : public RecVisitor {
    using RecVisitor::do_visit;
    using RecVisitor::enter_scope;
    using RecVisitor::visit;

public:
    ResolveDeps(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    bool visit(ast::Ref<ast::Root> node) override;
    bool visit(ast::Ref<ast::ModuleDef> node) override;
    bool do_visit(ast::Ref<ast::ClassDef> node) override;
    bool visit(ast::Ref<ast::TypeDef> node) override;
    bool visit(ast::Ref<ast::VarDefList> node) override;
    bool visit(ast::Ref<ast::FunDef> node) override;

    bool do_visit(ast::Ref<ast::TypeName> node) override;

protected:
    // start with corresponding object scope instead of a clean one
    void enter_module_scope(Ref<Module> module) override;
    void enter_class_scope(Ref<Class> cls) override;
    void enter_tpl_scope(Ref<ClassTpl> tpl) override;

private:
    struct ModuleTypeName {
        module_id_t module_id;
        ast::Ref<ast::TypeName> node;
    };

    void export_classes();

    std::list<ModuleTypeName> _unresolved;
};

} // namespace ulam::sema

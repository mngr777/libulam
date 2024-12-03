#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class Eval : public RecVisitor {
public:
    using RecVisitor::visit;

    Eval(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::VarDef> node) override;
    bool do_visit(ast::Ref<ast::FunDef> node) override;

protected:
    Ref<Type> resolve_type_def(ast::Ref<ast::TypeDef> node, Ref<Scope> scope);
    Ref<Type> resolve_type_name(ast::Ref<ast::TypeName> node, Ref<Scope> scope);
};

} // namespace ulam::sema

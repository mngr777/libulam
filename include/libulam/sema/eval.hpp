#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class Eval : public RecVisitor {
public:
    using RecVisitor::visit;

    Eval(Diag& diag, Ref<ast::Root> ast): RecVisitor{diag, ast, true} {}

    bool do_visit(Ref<ast::TypeDef> node) override;
    bool do_visit(Ref<ast::VarDef> node) override;
    bool do_visit(Ref<ast::FunDef> node) override;

protected:
    Ref<Type> resolve_type_def(Ref<ast::TypeDef> node, Ref<Scope> scope);
    Ref<Type> resolve_type_name(Ref<ast::TypeName> node, Ref<Scope> scope);
};

} // namespace ulam::sema

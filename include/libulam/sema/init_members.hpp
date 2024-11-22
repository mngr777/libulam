#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

class InitMembers : public sema::RecVisitor {
public:
    using RecVisitor::visit;

    InitMembers(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast, true} {}

    bool visit(ast::Ref<ast::VarDefList> node) override;
    bool do_visit(ast::Ref<ast::FunDef> node) override;
};

} // namespace ulam::sema

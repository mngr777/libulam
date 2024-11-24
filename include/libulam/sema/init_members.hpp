#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class InitMembers : public RecVisitor {
public:
    using RecVisitor::visit;

    InitMembers(Diag& diag, ast::Ref<ast::Root> ast): RecVisitor{diag, ast} {}

    bool visit(ast::Ref<ast::TypeName> node) override;

private:
    Ref<Type> resolve_type_name(ast::Ref<ast::TypeName> node);
};

} // namespace ulam::sema

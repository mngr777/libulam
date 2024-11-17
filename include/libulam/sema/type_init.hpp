#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast.hpp>
#include <libulam/sema/visitor.hpp>
#include <set>

namespace ulam::sema {

class TypeInit : public sema::RecVisitor {
public:
    explicit TypeInit(Sema& sema, ast::Ref<ast::Root> ast):
        RecVisitor{sema, ast} {}

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::TypeSpec> node) override;

private:
    std::set<str_id_t> _imports;
};

} // namespace ulam::sema

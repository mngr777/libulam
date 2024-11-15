#pragma once
#include "libulam/ast/nodes/module.hpp"
#include <libulam/ast.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

class TypeInit : public sema::RecVisitor {
public:
    explicit TypeInit(Sema& sema, ast::Ref<ast::Root> ast):
        RecVisitor{sema, ast} {}

    bool do_visit(ast::Ref<ast::ModuleDef> module) override;
    bool do_visit(ast::Ref<ast::ClassDef> class_def) override;
    // bool do_visit(ast::Ref<ast::TypeDef> type_def) override;
};

} // namespace ulam::sema

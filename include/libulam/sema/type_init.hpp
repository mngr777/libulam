#pragma once
#include <libulam/ast.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

class TypeInit : public Visitor {
public:
    explicit TypeInit(Sema& sema): Visitor{sema} {}

    bool do_visit(ast::Ref<ast::ModuleDef> module) override;
    bool do_visit(ast::Ref<ast::ClassDef> class_def) override;
};

} // namespace ulam::sema

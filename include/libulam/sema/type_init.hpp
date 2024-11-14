#pragma once
#include <libulam/ast.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

class TypeInit : public Visitor {
public:
    explicit TypeInit(Sema& sema): Visitor{sema} {}

    bool do_visit(ast::Module& module) override;
    bool do_visit(ast::ClassDef& class_def) override;
};

} // namespace ulam::sema

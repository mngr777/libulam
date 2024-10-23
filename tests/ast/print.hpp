#pragma once
#include "libulam/ast/nodes/expr.hpp"
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/nodes/params.hpp"
#include "libulam/ast/visitor.hpp"
#include <ostream>

namespace test::ast {

class PrinterBase : public ulam::ast::Visitor {
public:
    PrinterBase(std::ostream& os): _os{os} {}
#define NODE(str, cls) bool visit(ulam::ast::cls& node) override;
#include "libulam/ast/nodes.inc.hpp"
#undef NODE

    void print(ulam::ast::Node* node);

    struct {
        unsigned indent = 2;
    } options;

protected:
    std::ostream& indent();

    std::ostream& _os;
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os): PrinterBase{os} {}

    bool visit(ulam::ast::ClassDef& node) override;
    bool visit(ulam::ast::TypeDef& node) override;
    bool visit(ulam::ast::VarDef& node) override;
    bool visit(ulam::ast::Param& node) override; // TODO
    bool visit(ulam::ast::ParenExpr& node) override;
    bool visit(ulam::ast::BinaryOp& node) override;
    bool visit(ulam::ast::UnaryOp& node) override;
};

} // namespace test::ast

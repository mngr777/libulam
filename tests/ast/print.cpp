#include "print.hpp"
#include "libulam/ast/traversal.hpp"
#include "libulam/lang/ops.hpp"
#include <string>

namespace test::ast {

#define NODE(str, cls)                                                         \
    bool PrinterBase::visit(ulam::ast::cls& node) {                            \
        indent() << str << "\n";                                               \
        return true;                                                           \
    }
#include "libulam/ast/nodes.inc.hpp"
#undef NODE

void PrinterBase::print(ulam::ast::Node* node) {
    ulam::ast::traverse(node, *this);
}

std::ostream& PrinterBase::indent() {
    return _os << std::string(level() * options.indent, ' ');
}

bool Printer::visit(ulam::ast::ClassDef& node) {
    indent() << "ClassDef `" << node.name() << "'\n";
    return true;
}

bool Printer::visit(ulam::ast::TypeDef& node) {
    indent() << "TypeDef `" << node.alias() << "'\n";
    return true;
}

bool Printer::visit(ulam::ast::VarDef& node) {
    indent() << "VarDef `" << node.name() << "'" << (node.expr() ? " =" : "")
             << "\n";
    return true;
}

bool Printer::visit(ulam::ast::Param& node) {
    indent() << "Param `"<< node.name() <<"'\n";
    return true;
}

bool Printer::visit(ulam::ast::ParenExpr& node) {
    indent() << "(ParenExpr\n";
    inc_level();
    node.inner()->accept(*this);
    dec_level();
    indent() << ")\n";
    return false;
}

bool Printer::visit(ulam::ast::BinaryOp& node) {
    indent() << "(" << ulam::ops::str(node.op()) << "\n";
    inc_level();
    node.lhs()->accept(*this);
    node.rhs()->accept(*this);
    dec_level();
    indent() << ")\n";
    return false;
}

bool Printer::visit(ulam::ast::UnaryOp& node) {
    indent() << ulam::ops::str(node.op()) << "\n";
    inc_level();
    node.arg()->accept(*this);
    dec_level();
    indent() << ")\n";
    return false;
}

} // namespace test::ast

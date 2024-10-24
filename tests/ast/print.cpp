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
    indent() << "class `" << node.name() << "'\n";
    return true;
}

bool Printer::visit(ulam::ast::TypeDef& node) {
    indent() << "typedef `" << node.alias() << "'\n";
    return true;
}

bool Printer::visit(ulam::ast::VarDef& node) {
    indent() << "var `" << node.name() << "'" << (node.expr() ? " =" : "")
             << "\n";
    return true;
}

bool Printer::visit(ulam::ast::FunDef& node) {
    indent() << "fun `" << node.name() << "'\n";
    return true;
}

bool Printer::visit(ulam::ast::ParamList& node) {
    indent() << "(\n";
    for (unsigned n = 0; n < node.child_num(); ++n) {
        if (n > 0)
            _os << ",\n";
        node.child(n)->accept(*this);
    }
    indent() << ")\n";
    return false;
}

bool Printer::visit(ulam::ast::Param& node) {
    indent() << "`"<< node.name() <<"'\n";
    return true;
}

bool Printer::visit(ulam::ast::ParenExpr& node) {
    indent() << "(\n";
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

bool Printer::visit(ulam::ast::TypeName& node) {
    indent() << "TypeName `" << node.name() << "'\n";
    return false;
}

bool Printer::visit(ulam::ast::Name& node) {
    indent() << "Name `" << node.name() << "'\n";
    return false;
}

} // namespace test::ast

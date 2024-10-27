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

void PrinterBase::print(ulam::ast::Ref<ulam::ast::Node> node) {
    assert(node);
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
    indent() << "typedef ";
    accept_me(node.expr());
    _os << " " << node.alias() << "\n";
    return false;
}

bool Printer::visit(ulam::ast::VarDefList& node) {
    indent();
    accept_me(node.base_type());
    _os << " ";
    for (unsigned n = 0; n < node.def_num(); ++n) {
        if (n > 0)
            _os << ", ";
        accept_me(node.def(n));
    }
    _os << "\n";
    return false;
}

bool Printer::visit(ulam::ast::VarDef& node) {
    _os << node.name();
    auto value = node.value();
    if (value) {
        _os << " = ";
        accept_me(value);
    }
    return false;
}

bool Printer::visit(ulam::ast::FunDef& node) {
    indent();
    accept_me(node.ret_type());
    _os << " " << node.name();
    accept_me(node.params());
    _os << "\n";
    return false;
}

bool Printer::visit(ulam::ast::ParamList& node) {
    _os << "(";
    for (unsigned n = 0; n < node.child_num(); ++n) {
        if (n > 0)
            _os << ",";
        accept_me(node.child(n));
    }
    _os << ")";
    return false;
}

bool Printer::visit(ulam::ast::Param& node) {
    accept_me(node.type());
    _os << " " << node.name();
    if (node.default_value()) {
        _os << " = ";
        accept_me(node.default_value());
    }
    return true;
}

bool Printer::visit(ulam::ast::MemberAccess& node) {
    paren_l();
    accept_me(node.obj());
    _os << "." << node.mem_name();
    paren_r();
    return false;
}

bool Printer::visit(ulam::ast::ParenExpr& node) {
    _os << "(";
    accept_me(node.inner());
    _os << ")";
    return false;
}

bool Printer::visit(ulam::ast::BinaryOp& node) {
    paren_l();
    accept_me(node.lhs());
    _os << " " << ulam::ops::str(node.op()) << " ";
    accept_me(node.rhs());
    paren_r();
    return false;
}

bool Printer::visit(ulam::ast::UnaryOp& node) {
    paren_l();
    _os << ulam::ops::str(node.op());
    accept_me(node.arg());
    paren_r();
    return false;
}

bool Printer::visit(ulam::ast::TypeName& node) {
    _os << node.name();
    return false;
}

bool Printer::visit(ulam::ast::Name& node) {
    _os << node.name();
    return false;
}

bool Printer::visit(ulam::ast::Number& node) {
    _os << "<number>";
    return false;
}

bool Printer::visit(ulam::ast::String& node) {
    _os << "<string>";
    return false;
}

} // namespace test::ast

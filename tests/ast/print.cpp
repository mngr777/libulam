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
    return _no_indent ? _os : _os << std::string(level() * options.indent, ' ');
}

bool Printer::visit(ulam::ast::ClassDef& node) {
    indent() << "class `" << node.name() << "'" << nl();
    inc_level();
    assert(node.body());
    for (unsigned n = 0; n < node.body()->child_num(); ++n)
        accept_me(node.body()->child(n));
    dec_level();
    return false;
}

bool Printer::visit(ulam::ast::TypeDef& node) {
    indent() << "typedef ";
    accept_me(node.expr());
    _os << " " << node.alias() << ";" << nl();
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
    _os << ";" << nl();
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
    _os << nl();
    accept_me(node.body());
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

bool Printer::visit(ulam::ast::ArgList& node) {
    _os << "(";
    for (unsigned n = 0; n < node.child_num(); ++n) {
        if (n > 0)
            _os << ",";
        accept_me(node.child(n));
    }
    _os << ")";
    return false;
}

bool Printer::visit(ulam::ast::EmptyStmt& node) {
    indent() << "<empty-stmt>;" << nl();
    return false;
}

bool Printer::visit(ulam::ast::Block& node) {
    indent() << "{" << nl();
    inc_level();
    for (unsigned n = 0; n < node.child_num(); ++n)
        accept_me(node.child(n));
    dec_level();
    indent() << "}" << nl();
    return false;
}

bool Printer::visit(ulam::ast::If& node) {
    indent() << "if (";
    accept_me(node.cond());
    _os << ")" << nl();
    accept_me(node.if_branch());
    _os << "else" << nl();
    accept_me(node.else_branch());
    return false;
}

bool Printer::visit(ulam::ast::For& node) {
    indent() << "for (";
    bool no_indent = set_no_indent(true);
    bool no_newline = set_no_newline(true);
    accept_me(node.init());
    set_no_indent(no_indent);
    set_no_newline(no_newline);
    accept_me(node.cond());
    _os << "; ";
    accept_me(node.upd());
    _os << ")" << nl();
    accept_me(node.body());
    return false;
}

bool Printer::visit(ulam::ast::While& node) {
    indent() << "while (";
    accept_me(node.cond());
    _os << ")" << nl();
    accept_me(node.body());
    return false;
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
    for (unsigned n = 0; n < node.child_num(); ++n) {
        if (n > 0)
            _os << ".";
        accept_me(node.child(n));
    }
    return false;
}

bool Printer::visit(ulam::ast::TypeSpec& node) {
    accept_me(node.ident());
    if (node.args())
        accept_me(node.args());
    return false;
}

bool Printer::visit(ulam::ast::TypeIdent& node) {
    _os << node.name();
    return false;
}

bool Printer::visit(ulam::ast::Ident& node) {
    _os << node.name();
    return false;
}

bool Printer::visit(ulam::ast::NumLit& node) {
    _os << node.number().value;
    return false;
}

bool Printer::visit(ulam::ast::StrLit& node) {
    _os << "<string>";
    return false;
}

} // namespace test::ast

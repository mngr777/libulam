#include "print.hpp"
#include "libulam/ast/traversal.hpp"
#include "libulam/semantic/ops.hpp"
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
    indent() << "class `" << node.name_id() << "'";
    if (node.params())
        accept_me(node.params());
    _os << " {" << nl();
    inc_level();
    assert(node.body());
    for (unsigned n = 0; n < node.body()->child_num(); ++n)
        accept_me(node.body()->child(n));
    dec_level();
    indent() << "}" << nl();
    return false;
}

bool Printer::visit(ulam::ast::TypeDef& node) {
    indent() << "typedef ";
    {
        auto f = no_ws();
        accept_me(node.expr());
    }
    _os << " " << node.alias() << ";" << nl();
    return false;
}

bool Printer::visit(ulam::ast::VarDefList& node) {
    indent();
    {
        auto f = no_ws();
        accept_me(node.base_type());
        _os << " ";
        for (unsigned n = 0; n < node.def_num(); ++n) {
            if (n > 0)
                _os << ", ";
            accept_me(node.def(n));
        }
    }
    _os << ";" << nl();
    return false;
}

bool Printer::visit(ulam::ast::VarDef& node) {
    // no indent
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
    {
        auto f = no_ws();
        accept_me(node.ret_type());
        _os << " " << node.name();
        accept_me(node.params());
    }
    _os << nl();
    accept_me(node.body());
    return false;
}

bool Printer::visit(ulam::ast::ParamList& node) {
    // no indent
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
    // no indent
    auto f = no_ws();
    accept_me(node.type());
    _os << " " << node.name();
    if (node.default_value()) {
        _os << " = ";
        accept_me(node.default_value());
    }
    return true;
}

bool Printer::visit(ulam::ast::ArgList& node) {
    // no indent
    auto f = no_ws();
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
    {
        auto f = no_ws();
        accept_me(node.cond());
    }
    _os << ")" << nl();
    accept_me(node.if_branch());
    _os << "else" << nl();
    accept_me(node.else_branch());
    return false;
}

bool Printer::visit(ulam::ast::For& node) {
    indent() << "for (";
    {
        auto f = no_ws();
        accept_me(node.init());
        _os << " ";
        accept_me(node.cond());
        _os << "; ";
        accept_me(node.upd());
    }
    _os << ")" << nl();
    accept_me(node.body());
    return false;
}

bool Printer::visit(ulam::ast::While& node) {
    indent() << "while (";
    {
        auto f = no_ws();
        accept_me(node.cond());
    }
    _os << ")" << nl();
    accept_me(node.body());
    return false;
}

bool Printer::visit(ulam::ast::MemberAccess& node) {
    indent();
    paren_l();
    {
        auto f = no_ws();
        accept_me(node.obj());
    }
    _os << "." << node.mem_name();
    paren_r();
    _os << nl();
    return false;
}

bool Printer::visit(ulam::ast::ParenExpr& node) {
    indent();
    {
        auto f = no_ws();
        _os << "(";
        accept_me(node.inner());
        _os << ")";
    }
    _os << nl();
    return false;
}

bool Printer::visit(ulam::ast::BinaryOp& node) {
    indent();
    paren_l();
    {
        auto f = no_ws();
        accept_me(node.lhs());
        _os << " " << ulam::ops::str(node.op()) << " ";
        accept_me(node.rhs());
    }
    paren_r();
    _os << nl();
    return false;
}

bool Printer::visit(ulam::ast::UnaryPreOp& node) {
    indent();
    paren_l();
    _os << ulam::ops::str(node.op());
    {
        auto f = no_ws();
        accept_me(node.arg());
    }
    paren_r();
    _os << nl();
    return false;
}

bool Printer::visit(ulam::ast::UnaryPostOp& node) {
    indent();
    paren_l();
    {
        auto f = no_ws();
        accept_me(node.arg());
    }
    _os << ulam::ops::str(node.op());
    paren_r();
    _os << nl();
    return false;
}

bool Printer::visit(ulam::ast::TypeName& node) {
    // no indent
    auto f = no_ws();
    for (unsigned n = 0; n < node.child_num(); ++n) {
        if (n > 0)
            _os << ".";
        accept_me(node.child(n));
    }
    return false;
}

bool Printer::visit(ulam::ast::TypeSpec& node) {
    // no indent
    auto f = no_ws();
    accept_me(node.ident());
    if (node.args())
        accept_me(node.args());
    return false;
}

bool Printer::visit(ulam::ast::TypeIdent& node) {
    // no indent
    _os << node.name();
    return false;
}

bool Printer::visit(ulam::ast::Ident& node) {
    indent();
    _os << node.name() << nl();
    return false;
}

bool Printer::visit(ulam::ast::BoolLit& node) {
    indent();
    _os << (node.value() ? "true" : "false") << nl();
    return false;
}

bool Printer::visit(ulam::ast::NumLit& node) {
    indent();
    _os << node.value().value << nl();
    return false;
}

bool Printer::visit(ulam::ast::StrLit& node) {
    indent();
    const auto& value = node.value();
    _os << value.quote << value.value << value.quote << nl();
    return false;
}

} // namespace test::ast

#pragma once
#include "libulam/ast/nodes/stmt.hpp"
#include <libulam/ast.hpp>
#include <ostream>

namespace test::ast {

class PrinterBase : public ulam::ast::Visitor {
public:
    PrinterBase(std::ostream& os): _os{os} {}
#define NODE(str, cls) bool visit(ulam::ast::cls& node) override;
#include "libulam/ast/nodes.inc.hpp"
#undef NODE

    void print(ulam::ast::Ref<ulam::ast::Node> node);

    void paren_l() {
        if (options.explicit_parens)
            _os << "(";
    }

    void paren_r() {
        if (options.explicit_parens)
            _os << ")";
    }

    const char* nl() const { return _no_newline ? "" : "\n"; }

    struct {
        unsigned indent = 2;
        bool explicit_parens = true;
    } options;

protected:
    std::ostream& indent();

    class FormatOpts {
    public:
        FormatOpts(PrinterBase& printer, bool no_indent, bool no_newline):
            _printer(printer),
            _no_indent{printer.set_no_indent(no_indent)},
            _no_newline(printer.set_no_newline(no_newline)) {}
        ~FormatOpts() {
            _printer.set_no_indent(_no_indent);
            _printer.set_no_newline(_no_newline);
        }

    private:
        PrinterBase& _printer;
        bool _no_indent;
        bool _no_newline;
    };

    FormatOpts format(bool no_indent, bool no_newline) {
        return {*this, no_indent, no_newline};
    }

    FormatOpts no_ws() {
        return format(true, true);
    }

    bool set_no_indent(bool val) {
        bool cur = _no_indent;
        _no_indent = val;
        return cur;
    }

    bool set_no_newline(bool val) {
        bool cur = _no_newline;
        _no_newline = val;
        return cur;
    }

    std::ostream& _os;
    bool _no_newline{false};
    bool _no_indent{false};
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os): PrinterBase{os} {}

    bool visit(ulam::ast::ClassDef& node) override;
    bool visit(ulam::ast::TypeDef& node) override;
    bool visit(ulam::ast::VarDefList& node) override;
    bool visit(ulam::ast::VarDef& node) override;
    bool visit(ulam::ast::FunDef& node) override;
    bool visit(ulam::ast::ParamList& node) override;
    bool visit(ulam::ast::Param& node) override;
    bool visit(ulam::ast::ArgList& node) override;
    bool visit(ulam::ast::EmptyStmt& node) override;
    bool visit(ulam::ast::Block& node) override;
    bool visit(ulam::ast::If& node) override;
    bool visit(ulam::ast::For& node) override;
    bool visit(ulam::ast::While& node) override;
    // bool visit(ulam::ast::FunCall& node) override;
    bool visit(ulam::ast::MemberAccess& node) override;
    bool visit(ulam::ast::ParenExpr& node) override;
    bool visit(ulam::ast::BinaryOp& node) override;
    bool visit(ulam::ast::UnaryPreOp& node) override;
    bool visit(ulam::ast::UnaryPostOp& node) override;
    bool visit(ulam::ast::TypeName& node) override;
    bool visit(ulam::ast::TypeSpec& node) override;
    bool visit(ulam::ast::TypeIdent& node) override;
    bool visit(ulam::ast::Ident& node) override;
    bool visit(ulam::ast::NumLit& node) override;
    bool visit(ulam::ast::StrLit& node) override;

private:
    void accept_me(ulam::ast::Ref<ulam::ast::Node> node) {
        if (node) {
            node->accept(*this);
        } else {
            _os << "<empty>";
        }
    }
};

} // namespace test::ast

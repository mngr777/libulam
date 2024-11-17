#pragma once
#include "libulam/ast/nodes/stmt.hpp"
#include <libulam/ast.hpp>
#include <ostream>

namespace test::ast {

// TODO: use RecVisitor

class PrinterBase : public ulam::ast::Visitor {
public:
    PrinterBase(std::ostream& os): _os{os} {}
#define NODE(str, cls) bool visit(ulam::ast::Ref<ulam::ast::cls> node) override;
#include "libulam/ast/nodes.inc.hpp"
#undef NODE

    void print(
        ulam::ast::Ref<ulam::ast::Root> ast,
        ulam::ast::Ref<ulam::ast::Node> node);
    void print(ulam::ast::Ref<ulam::ast::Root> ast);

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
    const std::string_view name(ulam::ast::Ref<ulam::ast::Named> node) {
        return str(node->name_id());
    }
    const std::string_view str(ulam::str_id_t str_id);

    ulam::ast::Ref<ulam::ast::Root> ast() { return _ast; }

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

    FormatOpts no_ws() { return format(true, true); }

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

private:
    ulam::ast::Ref<ulam::ast::Root> _ast{};
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os): PrinterBase{os} {}

    bool visit(ulam::ast::Ref<ulam::ast::ClassDef> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::TypeDef> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::VarDefList> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::VarDef> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::FunDef> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::ParamList> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::Param> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::ArgList> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::EmptyStmt> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::Block> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::If> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::For> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::While> node) override;
    // bool visit(ulam::ast::Ref<ulam::ast::FunCall> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::MemberAccess> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::ParenExpr> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::BinaryOp> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::UnaryPreOp> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::UnaryPostOp> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::TypeName> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::TypeSpec> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::TypeIdent> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::Ident> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::BoolLit> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::NumLit> node) override;
    bool visit(ulam::ast::Ref<ulam::ast::StrLit> node) override;

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

#pragma once
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

    struct {
        unsigned indent = 2;
        bool explicit_parens = true;
    } options;

protected:
    std::ostream& indent();

    std::ostream& _os;
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os): PrinterBase{os} {}

    bool visit(ulam::ast::ClassDef& node) override;
    bool visit(ulam::ast::ClassDefBody& node) override;
    bool visit(ulam::ast::TypeDef& node) override;
    bool visit(ulam::ast::VarDefList& node) override;
    bool visit(ulam::ast::VarDef& node) override;
    bool visit(ulam::ast::FunDef& node) override;
    bool visit(ulam::ast::ParamList& node) override;
    bool visit(ulam::ast::Param& node) override;
    bool visit(ulam::ast::ArgList& node) override;
    // bool visit(ulam::ast::FunCall& node) override;
    bool visit(ulam::ast::MemberAccess& node) override;
    bool visit(ulam::ast::ParenExpr& node) override;
    bool visit(ulam::ast::BinaryOp& node) override;
    bool visit(ulam::ast::UnaryOp& node) override;
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

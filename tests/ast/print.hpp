#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/visitor.hpp>
#include <ostream>

namespace test::ast {

class PrinterBase : public ulam::ast::RecVisitor {
public:
    PrinterBase(std::ostream& os, ulam::ast::Ref<ulam::ast::Root> ast);
    virtual ~PrinterBase();

    void print();

    struct {
        unsigned ident = 2;
        bool expl_parens = true;
    } options;

protected:
    using RecVisitor::traverse;
    using RecVisitor::visit;

    // traversing containers
    void visit(ulam::ast::Ref<ulam::ast::ModuleDef> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::ModuleDef> node) override;
    void visit(ulam::ast::Ref<ulam::ast::ClassDefBody> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::ClassDefBody> node) override;
    void visit(ulam::ast::Ref<ulam::ast::FunDefBody> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::FunDefBody> node) override;
    void visit(ulam::ast::Ref<ulam::ast::Block> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::Block> node) override;

    void print_var_decl(ulam::ast::Ref<ulam::ast::VarDecl> node);
    void traverse_with_indent(ulam::ast::Ref<ulam::ast::Node> node);

    void accept_me(ulam::ast::Ref<ulam::ast::Node> node);
    void on_empty();

    std::ostream& indent();

    const char* paren_l();
    const char* paren_r();
    const char* nl();

    const std::string_view name(ulam::ast::Ref<ulam::ast::Named> node);
    const std::string_view str(ulam::str_id_t str_id);

    ulam::ast::Ref<ulam::ast::Root> ast();

    void inc_lvl();
    void dec_lvl();

    bool no_indent() { return _no_indent; }
    bool set_no_indent(bool val);

    bool no_newline() { return _no_newline; }
    bool set_no_newline(bool val);

    std::ostream& _os;

private:
    ulam::ast::Ref<ulam::ast::Root> _ast;

    unsigned _lvl{0};
    bool _no_newline{false};
    bool _no_indent{false};
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os, ulam::ast::Ref<ulam::ast::Root> ast):
        PrinterBase{os, ast} {}

protected:
    using PrinterBase::do_visit;
    using PrinterBase::traverse;
    using PrinterBase::visit;

    void visit(ulam::ast::Ref<ulam::ast::ClassDef> node) override;
    void visit(ulam::ast::Ref<ulam::ast::VarDef> node) override;
    void visit(ulam::ast::Ref<ulam::ast::FunDef> node) override;

    void visit(ulam::ast::Ref<ulam::ast::TypeSpec> node) override;
    void visit(ulam::ast::Ref<ulam::ast::ParamList> node) override;
    void visit(ulam::ast::Ref<ulam::ast::Param> node) override;
    void visit(ulam::ast::Ref<ulam::ast::ArgList> node) override;

    void visit(ulam::ast::Ref<ulam::ast::Block> node) override;
    void visit(ulam::ast::Ref<ulam::ast::EmptyStmt> node) override;
    void visit(ulam::ast::Ref<ulam::ast::If> node) override;
    void visit(ulam::ast::Ref<ulam::ast::For> node) override;
    void visit(ulam::ast::Ref<ulam::ast::Return> node) override;

    void visit(ulam::ast::Ref<ulam::ast::ParenExpr> node) override;
    void visit(ulam::ast::Ref<ulam::ast::BinaryOp> node) override;
    void visit(ulam::ast::Ref<ulam::ast::UnaryPreOp> node) override;
    void visit(ulam::ast::Ref<ulam::ast::UnaryPostOp> node) override;
    void visit(ulam::ast::Ref<ulam::ast::MemberAccess> node) override;

    bool do_visit(ulam::ast::Ref<ulam::ast::ModuleDef> node) override;
    bool do_visit(ulam::ast::Ref<ulam::ast::TypeDef> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::VarDefList> node) override;
    void traverse(ulam::ast::Ref<ulam::ast::TypeName> node) override;
    bool do_visit(ulam::ast::Ref<ulam::ast::Ident> node) override;
    bool do_visit(ulam::ast::Ref<ulam::ast::TypeIdent> node) override;

    bool do_visit(ulam::ast::Ref<ulam::ast::BoolLit> node) override;
    bool do_visit(ulam::ast::Ref<ulam::ast::NumLit> node) override;
    bool do_visit(ulam::ast::Ref<ulam::ast::StrLit> node) override;
};

} // namespace test::ast

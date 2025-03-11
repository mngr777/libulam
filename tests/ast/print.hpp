#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/access.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/visitor.hpp>
#include <ostream>

namespace test::ast {

class PrinterBase : public ulam::ast::RecVisitor {
public:
    PrinterBase(std::ostream& os, ulam::Ref<ulam::ast::Root> ast);
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
    void visit(ulam::Ref<ulam::ast::ModuleDef> node) override;
    void traverse(ulam::Ref<ulam::ast::ModuleDef> node) override;
    void visit(ulam::Ref<ulam::ast::ClassDefBody> node) override;
    void traverse(ulam::Ref<ulam::ast::ClassDefBody> node) override;
    void visit(ulam::Ref<ulam::ast::FunRetType> node) override;
    void visit(ulam::Ref<ulam::ast::FunDefBody> node) override;
    void traverse(ulam::Ref<ulam::ast::FunDefBody> node) override;
    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void traverse(ulam::Ref<ulam::ast::Block> node) override;

    void print_var_decl(ulam::Ref<ulam::ast::VarDecl> node);
    void print_array_dims(ulam::Ref<ulam::ast::ExprList> exprs);
    void traverse_with_indent(ulam::Ref<ulam::ast::Node> node);

    void accept_me(ulam::Ref<ulam::ast::Node> node);
    void on_empty();

    std::ostream& indent();

    const char* paren_l();
    const char* paren_r();
    const char* nl();

    const std::string_view name(ulam::Ref<ulam::ast::Named> node);
    const std::string_view str(ulam::str_id_t str_id);

    ulam::Ref<ulam::ast::Root> ast();

    void inc_lvl();
    void dec_lvl();

    bool no_indent() { return _no_indent; }
    bool set_no_indent(bool val);

    bool no_newline() { return _no_newline; }
    bool set_no_newline(bool val);

    std::ostream& _os;

private:
    ulam::Ref<ulam::ast::Root> _ast;

    unsigned _lvl{0};
    bool _no_newline{false};
    bool _no_indent{false};
};

class Printer : public PrinterBase {
public:
    Printer(std::ostream& os, ulam::Ref<ulam::ast::Root> ast):
        PrinterBase{os, ast} {}

protected:
    using PrinterBase::do_visit;
    using PrinterBase::traverse;
    using PrinterBase::visit;

    void visit(ulam::Ref<ulam::ast::ClassDef> node) override;
    void visit(ulam::Ref<ulam::ast::TypeDef> node) override;
    void visit(ulam::Ref<ulam::ast::VarDef> node) override;
    void visit(ulam::Ref<ulam::ast::FunDef> node) override;

    void visit(ulam::Ref<ulam::ast::TypeSpec> node) override;
    void visit(ulam::Ref<ulam::ast::TypeExpr> node) override;
    void visit(ulam::Ref<ulam::ast::FullTypeName> node) override;
    void visit(ulam::Ref<ulam::ast::ParamList> node) override;
    void visit(ulam::Ref<ulam::ast::Param> node) override;
    void visit(ulam::Ref<ulam::ast::ArgList> node) override;

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::EmptyStmt> node) override;
    void visit(ulam::Ref<ulam::ast::If> node) override;
    void visit(ulam::Ref<ulam::ast::IfAs> node) override;
    void visit(ulam::Ref<ulam::ast::For> node) override;
    void visit(ulam::Ref<ulam::ast::While> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
    void visit(ulam::Ref<ulam::ast::ExprStmt> node) override;

    void visit(ulam::Ref<ulam::ast::TypeOpExpr> node) override;
    void visit(ulam::Ref<ulam::ast::ParenExpr> node) override;
    void visit(ulam::Ref<ulam::ast::Cast> node) override;
    void visit(ulam::Ref<ulam::ast::BinaryOp> node) override;
    void visit(ulam::Ref<ulam::ast::UnaryOp> node) override;
    void visit(ulam::Ref<ulam::ast::ArrayAccess> node) override;
    void visit(ulam::Ref<ulam::ast::MemberAccess> node) override;
    void visit(ulam::Ref<ulam::ast::ClassConstAccess> node) override;

    bool do_visit(ulam::Ref<ulam::ast::ModuleDef> node) override;
    void traverse(ulam::Ref<ulam::ast::VarDefList> node) override;
    void traverse(ulam::Ref<ulam::ast::TypeName> node) override;
    bool do_visit(ulam::Ref<ulam::ast::Ident> node) override;
    bool do_visit(ulam::Ref<ulam::ast::TypeIdent> node) override;

    bool do_visit(ulam::Ref<ulam::ast::BoolLit> node) override;
    bool do_visit(ulam::Ref<ulam::ast::NumLit> node) override;
    bool do_visit(ulam::Ref<ulam::ast::StrLit> node) override;

    void traverse_list(ulam::Ref<ulam::ast::Node> node, std::string sep = ", ");
};

} // namespace test::ast

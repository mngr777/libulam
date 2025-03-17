#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/type.hpp>

namespace ulam::ast {

class Block : public List<Stmt, Stmt> {
    ULAM_AST_NODE
};

class ExprStmt : public Tuple<Stmt, Expr> {
    ULAM_AST_NODE
public:
    explicit ExprStmt(Ptr<Expr>&& expr): Tuple{std::move(expr)} {}

    ULAM_AST_TUPLE_PROP(expr, 0);
};

class If : public Tuple<Stmt, Expr, Stmt, Stmt> {
    ULAM_AST_NODE
public:
    If(Ptr<Expr>&& cond, Ptr<Stmt>&& if_branch, Ptr<Stmt>&& else_branch):
        Tuple{std::move(cond), std::move(if_branch), std::move(else_branch)} {}

    ULAM_AST_TUPLE_PROP(cond, 0)
    ULAM_AST_TUPLE_PROP(if_branch, 1)
    ULAM_AST_TUPLE_PROP(else_branch, 2)
};

// if (ident as Type)
class IfAs : public Tuple<Stmt, Ident, TypeName, Stmt, Stmt> {
    ULAM_AST_NODE
public:
    IfAs(
        Ptr<Ident>&& ident,
        Ptr<TypeName>&& type_name,
        Ptr<Stmt>&& if_branch,
        Ptr<Stmt>&& else_branch):
        Tuple{
            std::move(ident), std::move(type_name), std::move(if_branch),
            std::move(else_branch)} {}

    ULAM_AST_TUPLE_PROP(ident, 0)
    ULAM_AST_TUPLE_PROP(type_name, 1)
    ULAM_AST_TUPLE_PROP(if_branch, 2)
    ULAM_AST_TUPLE_PROP(else_branch, 3)
};

class For : public Tuple<Stmt, Stmt, Expr, Expr, Stmt> {
    ULAM_AST_NODE
public:
    For(Ptr<Stmt>&& init, Ptr<Expr>&& cond, Ptr<Expr>&& upd, Ptr<Stmt>&& body):
        Tuple{
            std::move(init), std::move(cond), std::move(upd), std::move(body)} {
    }

    ULAM_AST_TUPLE_PROP(init, 0)
    ULAM_AST_TUPLE_PROP(cond, 1)
    ULAM_AST_TUPLE_PROP(upd, 2)
    ULAM_AST_TUPLE_PROP(body, 3)
};

class While : public Tuple<Stmt, Expr, Stmt> {
    ULAM_AST_NODE
public:
    While(Ptr<Expr>&& cond, Ptr<Stmt>&& body):
        Tuple{std::move(cond), std::move(body)} {}

    ULAM_AST_TUPLE_PROP(cond, 0)
    ULAM_AST_TUPLE_PROP(body, 1)
};

class WhichCase : public Tuple<Stmt, Expr, Stmt> {
    ULAM_AST_NODE
public:
    WhichCase(Ptr<Expr>&& expr, Ptr<Stmt>&& branch):
        Tuple{std::move(expr), std::move(branch)} {}

    ULAM_AST_TUPLE_PROP(expr, 0)
    ULAM_AST_TUPLE_PROP(branch, 1)
};

class WhichCaseList : public List<Stmt, WhichCase> {
    ULAM_AST_NODE
};

class Which : public Tuple<Stmt, Expr, WhichCaseList> {
    ULAM_AST_NODE
public:
    Which(Ptr<Expr>&& expr): Tuple{std::move(expr), make<WhichCaseList>()} {}

    ULAM_AST_TUPLE_PROP(expr, 0)
    ULAM_AST_TUPLE_PROP(case_list, 1)
};

class Return : public Tuple<Stmt, Expr> {
    ULAM_AST_NODE
public:
    Return(Ptr<Expr>&& expr): Tuple{std::move(expr)} {}

    ULAM_AST_TUPLE_PROP(expr, 0)
};

class Break : public Stmt {
    ULAM_AST_NODE
};

class Continue : public Stmt {
    ULAM_AST_NODE
};

} // namespace ulam::ast

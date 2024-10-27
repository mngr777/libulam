#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>

namespace ulam::ast {

class _Stmt : public Node {};

class Stmt : public _Stmt {
    ULAM_AST_NODE
};

class Block : public ListOf<_Stmt, Block, Stmt> {
    ULAM_AST_NODE
};

class Branch : public OneOf<Stmt, Stmt, Block> {
    ULAM_AST_NODE
};

class If : public Tuple<Stmt, Expr, Branch, Branch> {
    ULAM_AST_NODE
public:
    If(Ptr<Expr>&& cond, Ptr<Branch>&& if_branch, Ptr<Branch>&& else_branch):
        Tuple{std::move(cond), std::move(if_branch), std::move(else_branch)} {}

    ULAM_AST_TUPLE_PROP(cond, 0);
    ULAM_AST_TUPLE_PROP(if_branch, 1);
    ULAM_AST_TUPLE_PROP(else_branch, 1);
};

class For : public Tuple<Stmt, Stmt, Expr, Expr, Branch> {
    ULAM_AST_NODE
public:
    For(Ptr<Stmt>&& init,
        Ptr<Expr>&& cond,
        Ptr<Expr>&& upd,
        Ptr<Branch>&& body):
        Tuple{
            std::move(init), std::move(cond), std::move(upd), std::move(body)} {
    }

    ULAM_AST_TUPLE_PROP(init, 0)
    ULAM_AST_TUPLE_PROP(cond, 1)
    ULAM_AST_TUPLE_PROP(upd, 2)
    ULAM_AST_TUPLE_PROP(body, 3)
};

class While : public Tuple<Stmt, Expr, Branch> {
public:
    While(Ptr<Expr>&& cond, Ptr<Branch>&& body):
        Tuple{std::move(cond), std::move(body)} {}

    ULAM_AST_TUPLE_PROP(cond, 0);
    ULAM_AST_TUPLE_PROP(body, 1);
};

} // namespace ulam::ast

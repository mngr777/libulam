#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/lang/ops.hpp>

namespace ulam::ast {

class Expr : public Node {};

class TypeName : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit TypeName(std::string&& name): Named{std::move(name)} {}
};

class Name : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit Name(std::string&& name): Named{std::move(name)} {}
};

class Ident : public Expr {};

class TypeIdent : public Ident {};

class ParenExpr : public Tuple<Expr, Expr> {
    ULAM_AST_NODE
public:
    ParenExpr(Ptr<Expr>&& inner): Tuple{std::move(inner)} {}

    ULAM_AST_TUPLE_PROP(inner, 0)
};

class OpExpr : public Expr {
public:
    explicit OpExpr(Op op): _op{op} {}

    Op op() const { return _op; }

protected:
    Op _op;
};

class BinaryOp : public Tuple<OpExpr, Expr, Expr> {
    ULAM_AST_NODE
public:
    BinaryOp(Op op, Ptr<Expr>&& lhs, Ptr<Expr>&& rhs):
        Tuple{std::move(lhs), std::move(rhs), op} {}

    ULAM_AST_TUPLE_PROP(lhs, 0)
    ULAM_AST_TUPLE_PROP(rhs, 1)
};

class UnaryOp : public Tuple<OpExpr, Expr> {
public:
    UnaryOp(Op op, Ptr<Expr>&& arg): Tuple{std::move(arg), op} {}

    ULAM_AST_TUPLE_PROP(arg, 0)
};

class VarRef : public Expr {};

class Cast : public Expr {};

class Bool : public Expr {};

class Number : public Expr {
    ULAM_AST_NODE
};

class String : public Expr {
    ULAM_AST_NODE
};

} // namespace ulam::ast

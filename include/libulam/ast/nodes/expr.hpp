#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/lang/ops.hpp>
#include <libulam/types.hpp>
#include <string>

namespace ulam::ast {

class Expr : public Node {};

class TypeName : public Expr {
    ULAM_AST_NODE
public:
    TypeName(std::string name): _name{std::move(name)} {}

    const std::string& name() const { return _name; }

private:
    std::string _name;
};

class Name : public Expr {
    ULAM_AST_NODE
public:
    Name(std::string name): _name{std::move(name)} {}

    const std::string& name() const { return _name; }

private:
    std::string _name;
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
    OpExpr(Op op): _op{op} {}

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

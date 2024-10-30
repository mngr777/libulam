#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/lang/number.hpp>
#include <libulam/lang/ops.hpp>

namespace ulam::ast {

class Expr : public Node {};

class TypeIdent; // TODO: TypeIdent
class Ident; // TODO: Ident
class ParenExpr;
class BinaryOp;
class UnaryOp;
class Cast;
class BoolLit;
class NumLit;
class StrLit;

class TypeIdent : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit TypeIdent(std::string&& name): Named{std::move(name)} {}
};

class Ident : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit Ident(std::string&& name): Named{std::move(name)} {}
};

class ParenExpr : public Tuple<Expr, Expr> {
    ULAM_AST_NODE
public:
    ParenExpr(Ptr<Expr>&& inner): Tuple{std::move(inner)} {}

    ULAM_AST_TUPLE_PROP(inner, 0)
};

class _OpExpr : public Expr {
public:
    explicit _OpExpr(Op op): _op{op} {}

    Op op() const { return _op; }

protected:
    Op _op;
};

class BinaryOp : public Tuple<_OpExpr, Expr, Expr> {
    ULAM_AST_NODE
public:
    BinaryOp(Op op, Ptr<Expr>&& lhs, Ptr<Expr>&& rhs):
        Tuple{std::move(lhs), std::move(rhs), op} {}

    ULAM_AST_TUPLE_PROP(lhs, 0)
    ULAM_AST_TUPLE_PROP(rhs, 1)
};

class UnaryOp : public Tuple<_OpExpr, Expr> {
public:
    UnaryOp(Op op, Ptr<Expr>&& arg): Tuple{std::move(arg), op} {}

    ULAM_AST_TUPLE_PROP(arg, 0)
};

class VarRef : public Expr {}; // ??

class Cast : public Tuple<Expr, Expr, Expr> {
    ULAM_AST_NODE
public:
    Cast(Ptr<Expr>&& type, Ptr<Expr>&& expr):
        Tuple{std::move(type), std::move(expr)} {}

    ULAM_AST_TUPLE_PROP(type, 0)
    ULAM_AST_TUPLE_PROP(expr, 1)
};

class BoolLit : public Expr {
    ULAM_AST_NODE
public:
    BoolLit(bool value): _value{value} {}

    bool value() const { return _value; }

private:
    bool _value;
};

class NumLit : public Expr {
    ULAM_AST_NODE
public:
    NumLit(Number&& number): _number{std::move(number)} {}

    const Number& number() const { return _number; }

private:
    Number _number;
};

class StrLit : public Expr {
    ULAM_AST_NODE
};

} // namespace ulam::ast

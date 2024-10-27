#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/lang/ops.hpp>

namespace ulam::ast {

class Expr : public Node {};

class TypeName;
class Name;
class ParenExpr;
class BinaryOp;
class UnaryOp;
class Cast;
class BoolLit;
class NumLit;
class StrLit;

using ExprVariant = Variant<
    TypeName,
    Name,
    ParenExpr,
    BinaryOp,
    UnaryOp,
    Cast,
    BoolLit,
    NumLit,
    StrLit>;

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
};

class StrLit : public Expr {
    ULAM_AST_NODE
};

} // namespace ulam::ast

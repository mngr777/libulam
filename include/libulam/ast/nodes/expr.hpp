#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/lang/number.hpp>
#include <libulam/lang/ops.hpp>
#include <libulam/lang/type_ops.hpp>

namespace ulam::ast {

class Expr : public Stmt {};

class ArgList;

class TypeIdent : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit TypeIdent(std::string&& name): Named{std::move(name)} {}
};

class TypeSpec : public Tuple<Expr, TypeIdent, ArgList> {
    ULAM_AST_NODE
public:
    TypeSpec(Ptr<TypeIdent>&& ident, Ptr<ArgList>&& args):
        Tuple{std::move(ident), std::move(args)} {}
    TypeSpec(Ptr<TypeIdent>&& ident): TypeSpec{std::move(ident), nullptr} {}

    ULAM_AST_TUPLE_PROP(ident, 0);
    ULAM_AST_TUPLE_PROP(args, 1);
};

class TypeName : public Tuple<ListOf<Expr, TypeIdent>, TypeSpec> {
    ULAM_AST_NODE
public:
    explicit TypeName(Ptr<TypeSpec>&& spec): Tuple{std::move(spec)} {}

    unsigned child_num() const override {
        return Tuple::child_num() + ListOf::child_num();
    }

    ULAM_AST_TUPLE_PROP(first, 0)

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : ListOf::child(n - 1);
    }

    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : ListOf::child(n - 1);
    }
};

class TypeOpExpr : public Tuple<Expr, TypeName> {
    ULAM_AST_NODE
public:
    TypeOpExpr(Ptr<TypeName>&& type, TypeOp op):
        Tuple{std::move(type)}, _op(op) {
        assert(op != TypeOp::None);
    }

    TypeOp op() const { return _op; }

private:
    TypeOp _op;
};

class Ident : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit Ident(std::string&& name): Named{std::move(name)} {}
};

class ParenExpr : public Tuple<Expr, Expr> {
    ULAM_AST_NODE
public:
    explicit ParenExpr(Ptr<Expr>&& inner): Tuple{std::move(inner)} {}

    ULAM_AST_TUPLE_PROP(inner, 0)
};

class _OpExpr : public Expr {
public:
    explicit _OpExpr(Op op): _op{op} { assert(op != Op::None); }

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

class Cast : public Tuple<Expr, TypeName, Expr> {
    ULAM_AST_NODE
public:
    Cast(Ptr<TypeName>&& type, Ptr<Expr>&& expr):
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

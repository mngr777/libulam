#pragma once
#include <cassert>
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/semantic/number.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/src_loc.hpp>

#define ULAM_AST_EXPR                                                          \
    ULAM_AST_NODE                                                              \
public:                                                                        \
    virtual ExprRes accept(ExprVisitor& v) override { return v.visit(this); }  \
                                                                               \
private:

namespace ulam::ast {

class Expr : public Stmt {
    ULAM_AST_NODE
public:
    virtual ExprRes accept(ExprVisitor& v) { return {}; };
};

class ExprList : public List<Node, Expr> {
    ULAM_AST_NODE
};

class TypeOpExpr : public Tuple<Expr, TypeName> {
    ULAM_AST_EXPR
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
    ULAM_AST_EXPR
public:
    Ident(Str name): Named{name} {}
};

class ParenExpr : public Tuple<Expr, Expr> {
    ULAM_AST_EXPR
public:
    explicit ParenExpr(Ptr<Expr>&& inner): Tuple{std::move(inner)} {}

    ULAM_AST_TUPLE_PROP(inner, 0)
};

class OpExpr : public Expr {
    ULAM_AST_SIMPLE_ATTR(loc_id_t, op_loc_id, NoLocId)
public:
    explicit OpExpr(Op op): _op{op} { assert(op != Op::None); }

    Op op() const { return _op; }

protected:
    Op _op;
};

class BinaryOp : public Tuple<OpExpr, Expr, Expr> {
    ULAM_AST_EXPR
public:
    BinaryOp(Op op, Ptr<Expr>&& lhs, Ptr<Expr>&& rhs):
        Tuple{std::move(lhs), std::move(rhs), op} {}

    ULAM_AST_TUPLE_PROP(lhs, 0)
    ULAM_AST_TUPLE_PROP(rhs, 1)
};

class _UnaryOp : public Tuple<OpExpr, Expr> {
public:
    _UnaryOp(Op op, Ptr<Expr>&& arg): Tuple{std::move(arg), op} {}

    ULAM_AST_TUPLE_PROP(arg, 0)
};

class UnaryPreOp : public _UnaryOp {
    ULAM_AST_EXPR
public:
    UnaryPreOp(Op op, Ptr<Expr>&& arg): _UnaryOp{op, std::move(arg)} {}
};

class UnaryPostOp : public _UnaryOp {
    ULAM_AST_EXPR
public:
    UnaryPostOp(Op op, Ptr<Expr>&& arg): _UnaryOp{op, std::move(arg)} {}
};

class VarRef : public Expr {}; // ??

class Cast : public Tuple<Expr, TypeName, Expr> {
    ULAM_AST_EXPR
public:
    Cast(Ptr<TypeName>&& type_name, Ptr<Expr>&& expr):
        Tuple{std::move(type_name), std::move(expr)} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)
    ULAM_AST_TUPLE_PROP(expr, 1)
};

template <typename T> class _Literal : public Expr {
public:
    _Literal(T&& value): _value{std::forward<T>(value)} {}

    const T& value() const { return _value; }

private:
    T _value;
};

class BoolLit : public _Literal<bool> {
    ULAM_AST_EXPR
public:
    BoolLit(bool value): _Literal{std::move(value)} {}
};

class NumLit : public _Literal<Number> {
    ULAM_AST_EXPR
public:
    NumLit(Number&& number): _Literal{std::move(number)} {}
};

class StrLit : public _Literal<String> {
    ULAM_AST_EXPR
public:
    StrLit(String&& string): _Literal{std::move(string)} {}
};

} // namespace ulam::ast

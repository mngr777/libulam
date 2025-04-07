#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/semantic/number.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/src_loc.hpp>

namespace ulam::ast {

// NOTE: type operators can work on both expressions and types, e.g.
// `T a; assert(T.maxof == a.maxof)
class TypeOpExpr : public Tuple<Expr, TypeName, Expr, TypeIdent, ArgList> {
    ULAM_AST_EXPR
public:
    TypeOpExpr(
        TypeOp op,
        Ptr<TypeName>&& type,
        Ptr<Expr>&& expr,
        Ptr<TypeIdent> base,
        Ptr<ArgList> args):
        Tuple{
            std::move(type), std::move(expr), std::move(base), std::move(args)},
        _op{op} {
        assert(op != TypeOp::None);
        assert(has_type_name() != has_expr());
        assert(has_expr() || !has_base());
    }

    ULAM_AST_TUPLE_PROP(type_name, 0)
    ULAM_AST_TUPLE_PROP(expr, 1)
    ULAM_AST_TUPLE_PROP(base, 2)
    ULAM_AST_TUPLE_PROP(args, 3)

    TypeOp op() const { return _op; }

private:
    TypeOp _op;
};

class Ident : public Expr, public Named {
    ULAM_AST_EXPR
    ULAM_AST_SIMPLE_ATTR(bool, is_self, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_super, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_local, false)
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

class UnaryOp : public Tuple<OpExpr, Expr, TypeName> {
    ULAM_AST_EXPR
public:
    UnaryOp(Op op, Ptr<Expr>&& arg, Ptr<TypeName>&& type_name = {}):
        Tuple{std::move(arg), std::move(type_name), op} {}

    ULAM_AST_TUPLE_PROP(arg, 0)
    ULAM_AST_TUPLE_PROP(type_name, 1);
};

class Cast : public Tuple<Expr, FullTypeName, Expr> {
    ULAM_AST_EXPR
public:
    Cast(Ptr<FullTypeName>&& full_type_name, Ptr<Expr>&& expr):
        Tuple{std::move(full_type_name), std::move(expr)} {}

    ULAM_AST_TUPLE_PROP(full_type_name, 0)
    ULAM_AST_TUPLE_PROP(expr, 1)
};

class Ternary : public Tuple<Expr, Expr, Expr, Expr> {
    ULAM_AST_EXPR
public:
    Ternary(Ptr<Expr>&& cond, Ptr<Expr>&& if_true, Ptr<Expr>&& if_false):
        Tuple{std::move(cond), std::move(if_true), std::move(if_false)} {}

    ULAM_AST_TUPLE_PROP(cond, 0)
    ULAM_AST_TUPLE_PROP(if_true, 1)
    ULAM_AST_TUPLE_PROP(if_false, 2)
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

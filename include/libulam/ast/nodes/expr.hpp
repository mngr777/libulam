#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/semantic/number.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/src_loc.hpp>

namespace ulam::ast {

class Expr : public Stmt {};

class ArgList;

// TODO: types are not expressions

class TypeIdent : public Expr, public Named {
    ULAM_AST_NODE
public:
    explicit TypeIdent(str_id_t name_id, loc_id_t name_loc_id):
        Named{name_id, name_loc_id} {}
};

class TypeSpec : public Tuple<Expr, TypeIdent, ArgList> {
    ULAM_AST_NODE
    ULAM_AST_LINK_ATTR(Type, base_type)
    ULAM_AST_LINK_ATTR(Type, type)
public:
    TypeSpec(Ptr<TypeIdent>&& ident, Ptr<ArgList>&& args):
        Tuple{std::move(ident), std::move(args)},
        _builtin_type_id{NoBuiltinTypeId} {}
    TypeSpec(BuiltinTypeId builtin_type_id, Ptr<ArgList>&& args):
        Tuple{{}, std::move(args)}, _builtin_type_id(builtin_type_id) {}

    bool is_builtin() const { return _builtin_type_id != NoBuiltinTypeId; }
    BuiltinTypeId builtin_type_id() const { return _builtin_type_id; }

    ULAM_AST_TUPLE_PROP(ident, 0);
    ULAM_AST_TUPLE_PROP(args, 1);

private:
    BuiltinTypeId _builtin_type_id;
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
    Ident(str_id_t name_id, loc_id_t name_loc_id):
        Named{name_id, name_loc_id} {}
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

class _UnaryOp : public Tuple<_OpExpr, Expr> {
public:
    _UnaryOp(Op op, Ptr<Expr>&& arg): Tuple{std::move(arg), op} {}

    ULAM_AST_TUPLE_PROP(arg, 0)
};

class UnaryPreOp : public _UnaryOp {
    ULAM_AST_NODE
public:
    UnaryPreOp(Op op, Ptr<Expr>&& arg): _UnaryOp{op, std::move(arg)} {}
};

class UnaryPostOp : public _UnaryOp {
    ULAM_AST_NODE
public:
    UnaryPostOp(Op op, Ptr<Expr>&& arg): _UnaryOp{op, std::move(arg)} {}
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

template <typename T> class _Literal : public Expr {
public:
    _Literal(T&& value): _value{value} {}

    const T& value() const { return _value; }

private:
    T _value;
};

class BoolLit : public _Literal<bool> {
    ULAM_AST_NODE
public:
    BoolLit(bool value): _Literal{std::move(value)} {}
};

class NumLit : public _Literal<Number> {
    ULAM_AST_NODE
public:
    NumLit(Number&& number): _Literal{std::move(number)} {}
};

class StrLit : public _Literal<String> {
    ULAM_AST_NODE
public:
    StrLit(String&& string): _Literal{std::move(string)} {}
};

} // namespace ulam::ast

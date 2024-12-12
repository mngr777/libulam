#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>

namespace ulam::ast {

class FunCall : public Tuple<OpExpr, Expr, ArgList> {
    ULAM_AST_EXPR
public:
    FunCall(Ptr<Expr>&& obj, Ptr<ArgList>&& args):
        Tuple{std::move(obj), std::move(args), Op::FunCall} {}

    ULAM_AST_TUPLE_PROP(obj, 0)
    ULAM_AST_TUPLE_PROP(args, 0)
};

class MemberAccess : public Tuple<OpExpr, Expr, Ident> {
    ULAM_AST_EXPR
public:
    MemberAccess(Ptr<Expr>&& obj, Ptr<Ident>&& ident):
        Tuple{std::move(obj), std::move(ident), Op::MemberAccess} {}

    ULAM_AST_TUPLE_PROP(obj, 0);
    ULAM_AST_TUPLE_PROP(ident, 1);
};

class ArrayAccess : public Tuple<OpExpr, Expr, Expr> {
    ULAM_AST_EXPR
public:
    ArrayAccess(Ptr<Expr>&& array, Ptr<Expr>&& index):
        Tuple{std::move(array), std::move(index), Op::ArrayAccess} {}

    ULAM_AST_TUPLE_PROP(array, 0)
    ULAM_AST_TUPLE_PROP(index, 1)
};

} // namespace ulam::ast

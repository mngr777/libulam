#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/args.hpp>
#include <libulam/ast/nodes/exprs.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/type.hpp>

namespace ulam::ast {

class FunCall : public Tuple<OpExpr, Expr, ArgList> {
    ULAM_AST_EXPR
    ULAM_AST_SIMPLE_ATTR(Op, fun_op, Op::None)
public:
    FunCall(Ptr<Expr>&& callable, Ptr<ArgList>&& args):
        Tuple{std::move(callable), std::move(args), Op::FunCall} {}

    ULAM_AST_TUPLE_PROP(callable, 0)
    ULAM_AST_TUPLE_PROP(args, 1)

    bool is_op_call() const { return fun_op() != Op::None; }

    unsigned arg_num() const {
        assert(has_args());
        return args()->child_num();
    }
};

class MemberAccess : public Tuple<OpExpr, Expr, Ident, TypeIdent> {
    ULAM_AST_EXPR
    ULAM_AST_SIMPLE_ATTR(Op, op, Op::None)
public:
    MemberAccess(
        Ptr<Expr>&& obj, Ptr<Ident>&& ident, Ptr<TypeIdent>&& base = {}):
        Tuple{
            std::move(obj), std::move(ident), std::move(base),
            Op::MemberAccess} {}

    bool is_op() const { return op() != Op::None; }

    ULAM_AST_TUPLE_PROP(obj, 0);
    ULAM_AST_TUPLE_PROP(ident, 1);
    ULAM_AST_TUPLE_PROP(base, 2);
};

class ClassConstAccess : public Tuple<OpExpr, TypeName, Ident> {
    ULAM_AST_EXPR
public:
    ClassConstAccess(Ptr<TypeName>&& type_name, Ptr<Ident>&& ident):
        Tuple{std::move(type_name), std::move(ident), Op::MemberAccess} {}

    ULAM_AST_TUPLE_PROP(type_name, 0);
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

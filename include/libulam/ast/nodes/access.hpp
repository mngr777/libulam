#pragma once
#include "libulam/ast/node.hpp"
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <utility>

namespace ulam::ast {

class FunCall : public Tuple<Expr, Expr, ArgList> {
    ULAM_AST_NODE
public:
    FunCall(Ptr<Expr>&& obj, Ptr<ArgList>&& args):
        Tuple{std::move(obj), std::move(args)} {}

    ULAM_AST_TUPLE_PROP(obj, 0)
    ULAM_AST_TUPLE_PROP(args, 0)
};

class ArrayAccess : public Tuple<Expr, Expr, Expr> {
    ULAM_AST_NODE
public:
    ArrayAccess(Ptr<Expr>&& array, Ptr<Expr>&& index):
        Tuple{std::move(array), std::move(index)} {}

    ULAM_AST_TUPLE_PROP(array, 0)
    ULAM_AST_TUPLE_PROP(index, 0)
};

} // namespace ulam::ast

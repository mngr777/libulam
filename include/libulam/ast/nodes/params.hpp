#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/src_loc.hpp>
#include <utility>

namespace ulam::ast {

class Param : public Tuple<VarDecl, TypeName> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
public:
    Param(
        Str name,
        Ptr<TypeName>&& type_name,
        Ptr<ExprList>&& array_dims,
        Ptr<Expr>&& value):
        Tuple{
            std::move(type_name), name, std::move(array_dims),
            std::move(value)} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)
};

class ParamList : public List<Node, Param> {
    ULAM_AST_NODE
};

class ArgList : public List<Node, Expr> {
    ULAM_AST_NODE
};

} // namespace ulam::ast

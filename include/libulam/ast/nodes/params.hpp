#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/src_loc.hpp>
#include <utility>

namespace ulam::ast {

class Param : public Tuple<Expr, TypeName, Expr>, public Named {
    ULAM_AST_NODE
public:
    Param(Str name, Ptr<TypeName>&& type, Ptr<Expr>&& default_value):
        Tuple{std::move(type), std::move(default_value)}, Named{name} {}

    ULAM_AST_TUPLE_PROP(type, 0)
    ULAM_AST_TUPLE_PROP(default_value, 1)
};

class ParamList : public List<Node, Param> {
    ULAM_AST_NODE
};

class ArgList : public List<Node, Expr> {
    ULAM_AST_NODE
};

} // namespace ulam::ast

#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>

namespace ulam::ast {

class ArgList : public List<Node, Expr> {
    ULAM_AST_NODE
};

} // namespace ulam::ast

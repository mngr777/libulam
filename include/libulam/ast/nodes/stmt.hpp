#pragma once
#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Stmt : public Node {};

class EmptyStmt : public Stmt {
    ULAM_AST_NODE
};

} // namespace ulam::ast

#pragma once
#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Stmt : public Node {};

class Block : public List<Stmt> {
    ULAM_AST_NODE
};

} // namespace ulam::ast

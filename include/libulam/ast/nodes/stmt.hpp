#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>

namespace ulam::ast {

class Stmt : public Node {};

class Block : public ListOf<Stmt, Block, Stmt> {
    ULAM_AST_NODE
};

class IfStmt : public Tuple<Expr> {

};

} // namespace ulam::ast

#pragma once
#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Stmt : public Node {
public:
    virtual bool is_empty() const { return false; }
    virtual bool is_block() const { return false; }
};

class EmptyStmt : public Stmt {
    ULAM_AST_NODE

    virtual bool is_empty() const override { return true; }
};

} // namespace ulam::ast

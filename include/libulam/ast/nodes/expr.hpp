#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/stmt.hpp>

#define ULAM_AST_EXPR                                                          \
    ULAM_AST_NODE                                                              \
public:                                                                        \
    virtual ExprRes accept(ExprVisitor& v) override { return v.visit(this); }  \
                                                                               \
private:

namespace ulam::ast {

class Expr : public Stmt {
    ULAM_AST_NODE
public:
    virtual ExprRes accept(ExprVisitor& v) { return {}; };
};

class ExprList : public List<Node, Expr> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, has_empty, false)
public:
    ExprList(): List{} {}

    void add(Ptr<Expr>&& expr) {
        if (!expr)
            set_has_empty(true);
        List::add(std::move(expr));
    }
};

} // namespace ulam::ast

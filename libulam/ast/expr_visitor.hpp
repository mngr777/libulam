#pragma once
#include <libulam/sema/expr_res.hpp>

namespace ulam::ast {

class Expr;

#define EXPR_NODE(name, cls) class cls;
#include <libulam/ast/expr_nodes.inc.hpp>

class ExprVisitor {
public:
    using ExprRes = sema::ExprRes;

#define EXPR_NODE(name, cls) virtual ExprRes visit(Ref<cls> node);
#include <libulam/ast/expr_nodes.inc.hpp>

protected:
    virtual ExprRes visit_default(Ref<Expr> node);
};

} // namespace ulam::ast

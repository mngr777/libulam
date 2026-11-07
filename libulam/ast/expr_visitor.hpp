#pragma once
#include <libulam/sema/expr_res.hpp>

namespace ulam::ast {

class Expr;

#define EXPR_NODE(name, cls) class cls;
#include <libulam/ast/expr_nodes.inc.hpp>

class ExprVisitor {
public:

#define EXPR_NODE(name, cls) virtual sema::ExprRes visit(Ref<cls> node);
#include <libulam/ast/expr_nodes.inc.hpp>

protected:
    virtual sema::ExprRes visit_default(Ref<Expr> node);
};

} // namespace ulam::ast

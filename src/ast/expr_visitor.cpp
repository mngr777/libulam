#include <libulam/assert.hpp>
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/nodes/exprs.hpp>

namespace ulam::ast {

using sema::ExprRes;

#define EXPR_NODE(name, cls)                                                   \
    ExprRes ExprVisitor::visit(Ref<cls> node) { return visit_default(node); }
#include <libulam/ast/expr_nodes.inc.hpp>

ExprRes ExprVisitor::visit_default(Ref<Expr> node) { unreachable(); }

} // namespace ulam::ast

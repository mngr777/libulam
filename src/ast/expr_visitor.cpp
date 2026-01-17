#include <libulam/assert.hpp>
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/nodes/exprs.hpp>

namespace ulam::ast {

#define DEF(type)                                                              \
    ExprVisitor::ExprRes ExprVisitor::visit(Ref<type> node) {                  \
        return visit_default(node);                                            \
    }

DEF(TypeOpExpr)
DEF(Ident)
DEF(ParenExpr)
DEF(BinaryOp)
DEF(UnaryOp)
DEF(Cast)
DEF(Ternary)
DEF(ClassName)
DEF(BoolLit)
DEF(NumLit)
DEF(StrLit)
DEF(FunCall)
DEF(MemberAccess)
DEF(ClassConstAccess)
DEF(ArrayAccess)

ExprVisitor::ExprRes ExprVisitor::visit_default(Ref<Expr> node) {
    unreachable();
}

} // namespace ulam::ast

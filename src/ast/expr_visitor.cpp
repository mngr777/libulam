#include <libulam/ast/nodes/exprs.hpp>
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>

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
DEF(BoolLit)
DEF(NumLit)
DEF(StrLit)
DEF(FunCall)
DEF(MemberAccess)
DEF(ClassConstAccess)
DEF(ArrayAccess)

ExprVisitor::ExprRes ExprVisitor::visit_default(Ref<Expr> node) {
    assert(false);
}

} // namespace ulam::ast

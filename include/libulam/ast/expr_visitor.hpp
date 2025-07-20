#pragma once
#include <libulam/sema/expr_res.hpp>

namespace ulam::ast {

class Expr;
class TypeOpExpr;
class Ident;
class ParenExpr;
class BinaryOp;
class UnaryOp;
class Cast;
class Ternary;
class BoolLit;
class NumLit;
class StrLit;
class FunCall;
class MemberAccess;
class ClassConstAccess;
class ArrayAccess;

class ExprVisitor {
public:
    using ExprRes = sema::ExprRes;

    virtual ExprRes visit(Ref<TypeOpExpr> node);
    virtual ExprRes visit(Ref<Ident> node);
    virtual ExprRes visit(Ref<ParenExpr> node);
    virtual ExprRes visit(Ref<UnaryOp> node);
    virtual ExprRes visit(Ref<BinaryOp> node);
    virtual ExprRes visit(Ref<Cast> node);
    virtual ExprRes visit(Ref<Ternary> node);
    virtual ExprRes visit(Ref<BoolLit> node);
    virtual ExprRes visit(Ref<NumLit> node);
    virtual ExprRes visit(Ref<StrLit> node);
    virtual ExprRes visit(Ref<FunCall> node);
    virtual ExprRes visit(Ref<MemberAccess> node);
    virtual ExprRes visit(Ref<ClassConstAccess> node);
    virtual ExprRes visit(Ref<ArrayAccess> node);

protected:
    virtual ExprRes visit_default(Ref<Expr> node);
};

} // namespace ulam::ast

#pragma once
#include <libulam/semantic/expr_res.hpp>

namespace ulam::ast {

class TypeOpExpr;
class Ident;
class ParenExpr;
class BinaryOp;
class UnaryPreOp;
class UnaryPostOp;
class VarRef;
class Cast;
class BoolLit;
class NumLit;
class StrLit;
class FunCall;
class MemberAccess;
class ArrayAccess;

class ExprVisitor {
public:
    virtual ExprRes visit(Ref<ast::TypeOpExpr> node) = 0;
    virtual ExprRes visit(Ref<ast::Ident> node) = 0;
    virtual ExprRes visit(Ref<ast::ParenExpr> node) = 0;
    virtual ExprRes visit(Ref<ast::BinaryOp> node) = 0;
    virtual ExprRes visit(Ref<ast::UnaryPreOp> node) = 0;
    virtual ExprRes visit(Ref<ast::UnaryPostOp> node) = 0;
    virtual ExprRes visit(Ref<ast::VarRef> node) = 0;
    virtual ExprRes visit(Ref<ast::Cast> node) = 0;
    virtual ExprRes visit(Ref<ast::BoolLit> node) = 0;
    virtual ExprRes visit(Ref<ast::NumLit> node) = 0;
    virtual ExprRes visit(Ref<ast::StrLit> node) = 0;
    virtual ExprRes visit(Ref<ast::FunCall> node) = 0;
    virtual ExprRes visit(Ref<ast::MemberAccess> node) = 0;
    virtual ExprRes visit(Ref<ast::ArrayAccess> node) = 0;
};

} // namespace ulam::ast

#pragma once
#include <libulam/sema/expr_res.hpp>

namespace ulam::ast {

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
    virtual sema::ExprRes visit(Ref<ast::TypeOpExpr> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::Ident> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::ParenExpr> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::UnaryOp> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::BinaryOp> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::Cast> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::Ternary> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::BoolLit> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::NumLit> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::StrLit> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::FunCall> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::MemberAccess> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::ClassConstAccess> node) = 0;
    virtual sema::ExprRes visit(Ref<ast::ArrayAccess> node) = 0;
};

} // namespace ulam::ast

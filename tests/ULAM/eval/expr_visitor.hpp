#pragma once
#include "./stringifier.hpp"
#include "./visitor.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

class EvalExprVisitor : public ulam::sema::EvalExprVisitor {
public:
    using Base = ulam::sema::EvalExprVisitor;
    using ExprError = ulam::sema::ExprError;
    using ExprRes = ulam::sema::ExprRes;

    EvalExprVisitor(
        EvalVisitor& eval,
        ulam::Ref<ulam::Program> program,
        ulam::Ref<ulam::Scope> scope,
        ulam::sema::eval_flags_t flags = ulam::sema::evl::NoFlags):
        Base::EvalExprVisitor{eval, program, scope, flags} {}

    ExprRes visit(ulam::Ref<ulam::ast::BoolLit> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::NumLit> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::StrLit> node) override;

protected:
    ExprRes apply_binary_op(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Op op,
        ulam::LValue lval,
        ulam::Ref<ulam::ast::Expr> l_node,
        ExprRes&& left,
        ulam::Ref<ulam::ast::Expr> r_node,
        ExprRes&& right) override;

    ExprRes apply_unary_op(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Op op,
        ulam::LValue lval,
        ulam::Ref<ulam::ast::Expr> arg_node,
        ExprRes&& arg,
        ulam::Ref<ulam::Type> type) override;

    ExprRes type_op_default(
        ulam::Ref<ulam::ast::TypeOpExpr> node,
        ulam::Ref<ulam::Type> type) override;

    ExprRes type_op_expr_default(
        ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) override;

    ExprRes ident_self(ulam::Ref<ulam::ast::Ident> node) override;

    ExprRes ident_super(ulam::Ref<ulam::ast::Ident> node) override;

    ExprRes ident_var(
        ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Var> var) override;

    ExprRes ident_prop(
        ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Prop> prop) override;

    ExprRes ident_fset(
        ulam::Ref<ulam::ast::Ident> node,
        ulam::Ref<ulam::FunSet> fset) override;

    ExprRes array_access_class(
        ulam::Ref<ulam::ast::ArrayAccess> node,
        ExprRes&& obj,
        ExprRes&& idx) override;

    ExprRes array_access_string(
        ulam::Ref<ulam::ast::ArrayAccess> node,
        ExprRes&& obj,
        ExprRes&& idx) override;

    ExprRes array_access_array(
        ulam::Ref<ulam::ast::ArrayAccess> node,
        ExprRes&& obj,
        ExprRes&& idx) override;

    ExprRes member_access_op(
        ulam::Ref<ulam::ast::MemberAccess> node, ExprRes&& obj) override;

    ExprRes member_access_var(
        ulam::Ref<ulam::ast::MemberAccess> node,
        ExprRes&& obj,
        ulam::Ref<ulam::Var> var) override;

    ExprRes member_access_prop(
        ulam::Ref<ulam::ast::MemberAccess> node,
        ExprRes&& obj,
        ulam::Ref<ulam::Prop> prop) override;

    ExprRes member_access_fset(
        ulam::Ref<ulam::ast::MemberAccess> node,
        ExprRes&& obj,
        ulam::Ref<ulam::FunSet> fset) override;

    ExprRes class_const_access(
        ulam::Ref<ulam::ast::ClassConstAccess> node,
        ulam::Ref<ulam::Var> var) override;

    ExprRes bind(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Ref<ulam::FunSet> fset,
        ulam::sema::ExprRes&& obj) override;

    ExprRes as_base(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Ref<ulam::ast::TypeIdent> base,
        ExprRes&& obj) override;

    Stringifier make_stringifier();
};

#pragma once
#include "./env.hpp"
#include "./helper.hpp"
#include "libulam/ast/nodes/expr.hpp"
#include "libulam/ast/nodes/type.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

class EvalExprVisitor : public EvalHelper, public ulam::sema::EvalExprVisitor {
public:
    using Base = ulam::sema::EvalExprVisitor;
    using ExprError = ulam::sema::ExprError;
    using ExprRes = ulam::sema::ExprRes;

    explicit EvalExprVisitor(EvalEnv& env): ::EvalHelper{env}, Base{env} {}

    ExprRes visit(ulam::Ref<ulam::ast::Cast> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::Ternary> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::BoolLit> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::NumLit> node) override;
    ExprRes visit(ulam::Ref<ulam::ast::StrLit> node) override;

protected:
    ExprRes apply_binary_op(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Op op,
        ExprRes&& lval_res,
        ulam::Ref<ulam::ast::Expr> l_node,
        ExprRes&& left,
        ulam::Ref<ulam::ast::Expr> r_node,
        ExprRes&& right) override;

    ExprRes apply_unary_op(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Op op,
        ExprRes&& lval_res,
        ulam::Ref<ulam::ast::Expr> arg_node,
        ExprRes&& arg,
        ulam::Ref<ulam::Type> type) override;

    ExprRes post_inc_dec_dummy() override;

    ExprRes type_op_construct(
        ulam::Ref<ulam::ast::TypeOpExpr> node,
        ulam::Ref<ulam::Class> cls) override;

    ExprRes type_op_default(
        ulam::Ref<ulam::ast::TypeOpExpr> node,
        ulam::Ref<ulam::Type> type) override;

    ExprRes type_op_expr_construct(
        ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) override;

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
        ulam::Ref<ulam::FunSet> fset,
        ulam::Ref<ulam::Class> base) override;

    ExprRes class_const_access(
        ulam::Ref<ulam::ast::ClassConstAccess> node,
        ulam::Ref<ulam::Var> var) override;

    ExprRes bind(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Ref<ulam::FunSet> fset,
        ulam::sema::ExprRes&& obj,
        ulam::Ref<ulam::Class> base) override;

    ExprRes negate(ulam::Ref<ulam::ast::Expr> node, ExprRes&& res) override;

    ulam::Ref<ulam::Class> class_base_ident(
        ulam::Ref<ulam::ast::Expr> node,
        ExprRes& obj,
        ulam::Ref<ulam::Class> cls,
        ulam::Ref<ulam::ast::TypeIdent> ident) override;

    ulam::Ref<ulam::Class> class_base_classid(
        ulam::Ref<ulam::ast::Expr> expr,
        ExprRes& obj,
        ulam::Ref<ulam::Class> cls,
        ExprRes&& classid) override;
};

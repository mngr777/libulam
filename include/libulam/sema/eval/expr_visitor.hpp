#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <utility>

namespace ulam {
class Program;
}

namespace ulam::sema {

class EvalVisitor;

class EvalExprVisitor : public EvalHelper, public ast::ExprVisitor {
    friend EvalVisitor;
public:
    EvalExprVisitor(
        EvalVisitor& eval,
        Ref<Program> program,
        Ref<Scope> scope,
        eval_flags_t flags):
        EvalHelper(eval, program, scope, flags) {}

    EvalExprVisitor(EvalExprVisitor&&) = default;
    EvalExprVisitor& operator=(EvalExprVisitor&& other);

    ExprRes visit(Ref<ast::TypeOpExpr> node) override;
    ExprRes visit(Ref<ast::Ident> node) override;
    ExprRes visit(Ref<ast::ParenExpr> node) override;
    ExprRes visit(Ref<ast::BinaryOp> node) override;
    ExprRes visit(Ref<ast::UnaryOp> node) override;
    ExprRes visit(Ref<ast::Cast> node) override;
    ExprRes visit(Ref<ast::Ternary> node) override;
    ExprRes visit(Ref<ast::BoolLit> node) override;
    ExprRes visit(Ref<ast::NumLit> node) override;
    ExprRes visit(Ref<ast::StrLit> node) override;
    ExprRes visit(Ref<ast::FunCall> node) override;
    ExprRes visit(Ref<ast::MemberAccess> node) override;
    ExprRes visit(Ref<ast::ClassConstAccess> node) override;
    ExprRes visit(Ref<ast::ArrayAccess> node) override;

    virtual ExprRes check(Ref<ast::Expr> node, ExprRes&& res);

    virtual bool check_is_assignable(Ref<ast::Expr> node, const Value& value);
    virtual bool check_is_object(
        Ref<ast::Expr> node, Ref<const Type> type, bool deref = false);
    virtual bool check_is_class(
        Ref<ast::Expr> node, Ref<const Type> type, bool deref = false);

    virtual Ref<Class> class_super(Ref<ast::Expr> node, Ref<Class> cls);
    virtual Ref<Class>
    class_base(Ref<ast::Expr> node, Ref<Class> cls, Ref<ast::TypeIdent> ident);

    // {res, ok}
    virtual std::pair<bool, bool>
    match(Ref<ast::Expr> var_expr, Ref<Var> var, Ref<ast::Expr> expr);

    // TODO: move to Resolver

    virtual bitsize_t
    bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id);

    virtual array_size_t array_size(Ref<ast::Expr> expr);

    virtual std::pair<TypedValueList, bool>
    eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl);

protected:
    virtual ExprRes binary_op(
        Ref<ast::Expr> node,
        Op op,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

    virtual ExprRes apply_binary_op(
        Ref<ast::Expr> node,
        Op op,
        LValue lval,
        Ref<ast::Expr> l_node,
        ExprRes&& left,
        Ref<ast::Expr> r_node,
        ExprRes&& right);

    virtual ExprRes unary_op(
        Ref<ast::Expr> node,
        Op op,
        Ref<ast::Expr> arg_node,
        ExprRes&& arg,
        Ref<ast::TypeName> type_name = {});

    virtual ExprRes apply_unary_op(
        Ref<ast::Expr> node,
        Op op,
        LValue lval,
        Ref<ast::Expr> arg_node,
        ExprRes&& arg,
        Ref<Type> type = {});

    virtual ExprRes type_op(Ref<ast::TypeOpExpr> node, Ref<Type> type);
    virtual ExprRes
    type_op_construct(Ref<ast::TypeOpExpr> node, Ref<Class> cls);
    virtual ExprRes type_op_default(Ref<ast::TypeOpExpr> node, Ref<Type> type);

    virtual ExprRes type_op_expr(Ref<ast::TypeOpExpr> node, ExprRes&& arg);

    virtual ExprRes type_op_expr_fun(
        Ref<ast::TypeOpExpr> node, Ref<FunSet> fset, ExprRes&& arg);
    virtual ExprRes
    type_op_expr_default(Ref<ast::TypeOpExpr> node, ExprRes&& arg);

    virtual ExprRes ident_self(Ref<ast::Ident> node);
    virtual ExprRes ident_super(Ref<ast::Ident> node);
    virtual ExprRes ident_var(Ref<ast::Ident> node, Ref<Var> var);
    virtual ExprRes ident_prop(Ref<ast::Ident> node, Ref<Prop> prop);
    virtual ExprRes ident_fset(Ref<ast::Ident> node, Ref<FunSet> fset);

    virtual ExprRes callable_op(Ref<ast::FunCall> node);

    virtual ExprRes array_access_class(
        Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx);
    virtual ExprRes array_access_string(
        Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx);
    virtual ExprRes array_access_array(
        Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx);

    virtual ExprRes
    member_access_op(Ref<ast::MemberAccess> node, ExprRes&& obj);
    virtual ExprRes
    member_access_var(Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Var> var);
    virtual ExprRes member_access_prop(
        Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Prop> prop);
    virtual ExprRes member_access_fset(
        Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<FunSet> fset);

    virtual ExprRes
    class_const_access(Ref<ast::ClassConstAccess> node, Ref<Var> var);

    virtual ExprRes bind(Ref<ast::Expr> node, Ref<FunSet> fset, ExprRes&& obj);

    virtual ExprRes
    as_base(Ref<ast::Expr> node, Ref<ast::TypeIdent> base, ExprRes&& obj);

    virtual ExprRes
    assign(Ref<ast::Expr> node, TypedValue&& to, TypedValue&& tv);

    virtual ExprResList eval_args(Ref<ast::ArgList> args);
};

} // namespace ulam::sema

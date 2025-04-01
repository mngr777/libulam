#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/diag.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>
#include <utility>

namespace ulam {
class Program;
}

namespace ulam::sema {

class EvalVisitor;

class EvalExprVisitor : public ast::ExprVisitor {
public:
    EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope);

    EvalExprVisitor(EvalExprVisitor&&) = default;
    EvalExprVisitor& operator=(EvalExprVisitor&& other);

    virtual ExprRes visit(Ref<ast::TypeOpExpr> node) override;
    virtual ExprRes visit(Ref<ast::Ident> node) override;
    virtual ExprRes visit(Ref<ast::ParenExpr> node) override;
    virtual ExprRes visit(Ref<ast::BinaryOp> node) override;
    virtual ExprRes visit(Ref<ast::UnaryOp> node) override;
    virtual ExprRes visit(Ref<ast::Cast> node) override;
    virtual ExprRes visit(Ref<ast::Ternary> node) override;
    virtual ExprRes visit(Ref<ast::BoolLit> node) override;
    virtual ExprRes visit(Ref<ast::NumLit> node) override;
    virtual ExprRes visit(Ref<ast::StrLit> node) override;
    virtual ExprRes visit(Ref<ast::FunCall> node) override;
    virtual ExprRes visit(Ref<ast::MemberAccess> node) override;
    virtual ExprRes visit(Ref<ast::ClassConstAccess> node) override;
    virtual ExprRes visit(Ref<ast::ArrayAccess> node) override;

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

    virtual ExprRes
    assign(Ref<ast::Expr> node, TypedValue&& to, TypedValue&& tv);

    virtual std::pair<TypedValueList, ExprError>
    eval_args(Ref<ast::ArgList> args);

    Diag& diag();
    Builtins& builtins();

    std::string_view str(str_id_t str_id);

private:
    EvalVisitor& _eval;
    Ref<Program> _program;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

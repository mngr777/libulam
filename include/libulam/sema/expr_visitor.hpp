#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>
#include <utility>

// TODO: option for allowing/disallowing empty (unknown) values

namespace ulam {
class Program;
}

namespace ulam::sema {

class ExprVisitor : public ast::ExprVisitor {
public:
    enum CastStatus { CastOk, CastError, NoCast };
    using CastRes = std::pair<RValue, CastStatus>;

    ExprVisitor(Ref<Program> program, Ref<Scope> scope):
        _program{program}, _scope{scope} {}

    ExprVisitor(ExprVisitor&&) = default;
    ExprVisitor& operator=(ExprVisitor&& other);

    virtual ExprRes visit(Ref<ast::TypeOpExpr> node) override;
    virtual ExprRes visit(Ref<ast::Ident> node) override;
    virtual ExprRes visit(Ref<ast::ParenExpr> node) override;
    virtual ExprRes visit(Ref<ast::BinaryOp> node) override;
    virtual ExprRes visit(Ref<ast::UnaryOp> node) override;
    virtual ExprRes visit(Ref<ast::Cast> node) override;
    virtual ExprRes visit(Ref<ast::BoolLit> node) override;
    virtual ExprRes visit(Ref<ast::NumLit> node) override;
    virtual ExprRes visit(Ref<ast::StrLit> node) override;
    virtual ExprRes visit(Ref<ast::FunCall> node) override;
    virtual ExprRes visit(Ref<ast::MemberAccess> node) override;
    virtual ExprRes visit(Ref<ast::ArrayAccess> node) override;

    virtual bool check_is_assignable(Ref<ast::Expr> node, const Value& value);
    virtual bool check_is_object(
        Ref<ast::Expr> node, Ref<const Type> type, bool deref = false);
    virtual bool check_is_class(
        Ref<ast::Expr> node, Ref<const Type> type, bool deref = false);

    virtual ExprRes
    cast(Ref<ast::Expr> node, Ref<Type> type, ExprRes&& res, bool expl);

    virtual bitsize_t bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id);

    virtual array_idx_t array_index(Ref<ast::Expr> expr);

    // TODO: return type, refactoring
    virtual std::pair<TypedValueList, bool> eval_args(Ref<ast::ArgList> args);

protected:
    virtual ExprRes assign(Ref<ast::OpExpr> node, Value&& val, TypedValue&& tv);

    virtual CastRes maybe_cast(
        Ref<ast::Expr> node,
        Ref<Type> type,
        TypedValue&& tv,
        bool expl = false);

    RValue do_cast(Ref<ast::Expr> node, Ref<const Type> type, TypedValue&& tv);

    TypedValue do_cast(
        Ref<ast::Expr> node, BuiltinTypeId builtin_type_id, TypedValue&& tv);

    virtual ExprRes funcall(
        Ref<ast::Expr> node,
        Ref<FunSet> fset,
        LValue self,
        TypedValueList&& args);

    virtual ExprRes funcall(
        Ref<ast::Expr> node, Ref<Fun> fun, LValue self, TypedValueList&& args);

    Diag& diag();
    Builtins& builtins();

    std::string_view str(str_id_t str_id);

private:
    Ref<Program> _program;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

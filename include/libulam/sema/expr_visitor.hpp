#pragma once
#include <libulam/ast/expr_visitor.hpp>
#include <libulam/ast/nodes.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/prim/ops.hpp>
#include <libulam/semantic/type/prim/typed_value.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>
#include <utility>

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

    virtual ExprRes cast(
        loc_id_t loc_id,
        std::size_t len,
        ExprRes&& res,
        Ref<Type> type,
        bool is_expl);

    virtual array_idx_t array_index(Ref<ast::Expr> expr);

protected:
    virtual ExprRes prim_unary_op(Ref<ast::UnaryOp> node, PrimTypedValue&& arg);

    virtual ExprRes prim_binary_op(
        Ref<ast::BinaryOp> node, PrimTypedValue&& left, PrimTypedValue&& right);

    virtual ExprRes array_binary_op(
        Ref<ast::BinaryOp> node, TypedValue&& left, TypedValue&& right);

    virtual ExprRes assign(Ref<ast::OpExpr> node, Value&& val, TypedValue&& tv);

    virtual CastRes maybe_cast(
        Ref<ast::Expr> node,
        Ref<Type> type,
        TypedValue&& tv,
        bool expl = false);

    RValue do_cast(Ref<Type> type, TypedValue&& tv);

    virtual PrimTypedValue
    prim_cast(BuiltinTypeId type_id, PrimTypedValue&& tv);

    virtual RValue prim_cast(Ref<PrimType> type, PrimTypedValue&& tv);

    virtual ExprRes funcall(
        Ref<ast::FunCall> node,
        Ref<Fun> fun,
        ObjectView obj_view,
        TypedValueList&& args);

    // TODO: return type, refactoring
    virtual std::pair<TypedValueList, bool> eval_args(Ref<ast::ArgList> args);

    Diag& diag();
    Builtins& builtins();

    std::string_view str(str_id_t str_id);

private:
    Ref<Program> _program;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

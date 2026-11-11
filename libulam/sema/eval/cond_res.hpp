#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/types.hpp>
#include <utility>

namespace ulam::sema {

// TODO: add ast::AsCond for codegen, see tests/ULAM/eval/which.cpp
class AsCondContext {
public:
    // (a as A), match
    AsCondContext(Ref<Type> type, Ptr<ast::VarDef>&& var_def, Ptr<Var>&& var):
        _type{type}, _var_def{std::move(var_def)}, _var{std::move(var)} {}

    // (self as A), match
    AsCondContext(Ref<Type> type, LValue self):
        _type{type}, _self{self}, _is_self{true} {}

    // not a match
    AsCondContext(Ref<Type> type): _type{type} {}

    // empty
    AsCondContext() {}

    AsCondContext(AsCondContext&&) = default;
    AsCondContext& operator=(AsCondContext&&) = default;

    bool empty() const { return !has_var() && !is_self(); }

    Ref<Type> type() const { return _type; }

    bool has_var() const { return ref(_var); }
    Ref<Var> var() const {
        ulam_assert(has_var());
        return ref(_var);
    }

    bool is_self() const { return _is_self; }
    LValue self() const {
        ulam_assert(_is_self);
        return _self;
    }

private:
    Ref<Type> _type{};
    Ptr<ast::VarDef> _var_def;
    Ptr<Var> _var;
    LValue _self;
    bool _is_self{false};
};

class CondRes {
public:
    CondRes(OptBool truth, ExprRes&& expr_res):
        _truth{truth}, _expr_res{std::move(expr_res)} {}

    CondRes(OptBool truth, AsCondContext&& as_cond_ctx):
        _truth{truth}, _as_cond_ctx{std::move(as_cond_ctx)} {}

    const OptBool& truth() { return _truth; }

    const ExprRes& expr_res() const { return _expr_res; }
    ExprRes move_expr_res() { return std::exchange(_expr_res, {}); }

    const AsCondContext& as_cond_ctx() const { return _as_cond_ctx; }
    AsCondContext& as_cond_ctx() { return _as_cond_ctx; }
    AsCondContext move_as_cond_ctx() { return std::exchange(_as_cond_ctx, {}); }

private:
    OptBool _truth;
    ExprRes _expr_res;
    AsCondContext _as_cond_ctx;
};

} // namespace ulam::sema

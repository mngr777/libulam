#pragma once
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/type.hpp>
#include <utility>

namespace ulam::sema {

class EvalCond : public EvalHelper {
public:
    using EvalHelper::EvalHelper;

    class AsCondContext {
    public:
        AsCondContext(Ptr<ast::VarDef>&& var_def, Ref<Var> var):
            _var_def{std::move(var_def)}, _var{var} {
            assert(_var_def && _var);
        }
        AsCondContext(): _var_def{}, _var{} {}

        AsCondContext(AsCondContext&&) = default;
        AsCondContext& operator=(AsCondContext&&) = default;

        bool has_var() const { return _var; }
        Ref<Var> var() const { return _var; }

    private:
        Ptr<ast::VarDef> _var_def;
        Ref<Var> _var;
    };

    using CondRes = std::pair<bool, AsCondContext>;

    virtual CondRes eval_cond(Ref<ast::Cond> cond);

    virtual CondRes
    eval_as_cond(Ref<ast::AsCond> as_cond);

protected:
    using VarDefPair = std::pair<Ptr<ast::VarDef>, Ref<Var>>;

    virtual CondRes eval_expr(Ref<ast::Expr> expr);

    virtual ExprRes eval_as_cond_ident(Ref<ast::Ident> ident);

    virtual Ref<Type>
    resolve_as_cond_type(Ref<ast::TypeName> type_name);

    virtual VarDefPair define_as_cond_var(
        Ref<ast::AsCond> node,
        ExprRes&& res,
        Ref<Type> type);
};

} // namespace ulam::sema

#pragma once
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class EvalCond : public EvalHelper {
public:
    using EvalHelper::EvalHelper;

    virtual CondRes eval_cond(Ref<ast::Cond> cond);

    virtual CondRes eval_as_cond(Ref<ast::AsCond> as_cond);

protected:
    using VarDefPair = std::pair<Ptr<ast::VarDef>, Ptr<Var>>;

    virtual CondRes eval_expr(Ref<ast::Expr> expr);

    virtual ExprRes eval_as_cond_ident(Ref<ast::Ident> ident);

    virtual Ref<Type> resolve_as_cond_type(Ref<ast::TypeName> type_name);

    virtual LValue
    as_cond_lvalue(Ref<ast::AsCond> node, ExprRes&& res, Ref<Type> type);

    virtual VarDefPair
    make_as_cond_var(Ref<ast::AsCond> node, ExprRes&& res, Ref<Type> type);
};

} // namespace ulam::sema

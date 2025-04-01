#pragma once
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalFuncall {
public:
    EvalFuncall(EvalVisitor& eval, Diag& diag, Ref<Scope> scope):
        _eval{eval}, _diag{diag}, _scope{scope} {}

    virtual ExprRes funcall(
        Ref<ast::Node> node,
        Ref<FunSet> fset,
        LValue self,
        TypedValueList&& args);

    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<Fun> fun, LValue self, TypedValueList&& args);

protected:
    EvalVisitor& _eval;
    Diag& _diag;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

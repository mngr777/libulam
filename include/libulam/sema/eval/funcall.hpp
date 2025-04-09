#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/typed_value.hpp>

// TODO: interface

namespace ulam::sema {

class EvalVisitor;

class EvalFuncall {
public:
    EvalFuncall(EvalVisitor& eval, Diag& diag, Ref<Scope> scope):
        _eval{eval}, _diag{diag}, _scope{scope} {}

    virtual ExprRes
    funcall(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args);

    // TODO: remove
    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<FunSet> fset, LValue self, ExprResList&& args);

    // TODO: remove
    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<Fun> fun, LValue self, TypedValueList&& args);

protected:
    virtual ExprRes do_funcall(
        Ref<ast::Node> node,
        Ref<Fun> fun,
        ExprRes&& callable,
        ExprResList&& args);

    virtual std::pair<FunSet::Matches, ExprError> find_match(
        Ref<ast::Node> node,
        Ref<FunSet> fset,
        Ref<Class> dyn_cls,
        const TypedValueRefList& args);

    virtual ExprResList
    cast_args(Ref<ast::Node>, Ref<Fun> fun, ExprResList&& args);

    EvalVisitor& _eval;
    Diag& _diag;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/typed_value.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalFuncall : public EvalHelper {
public:
    using EvalHelper::EvalHelper;

    virtual ExprRes
    construct(Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args);

    virtual ExprRes
    call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args);

    virtual ExprRes funcall(
        Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args);

protected:
    virtual ExprRes construct_funcall(
        Ref<ast::Node> node,
        Ref<Class> cls,
        Ref<Fun> fun,
        RValue&& rval,
        ExprResList&& args);

    virtual ExprRes funcall_callable(
        Ref<ast::Node> node,
        Ref<Fun> fun,
        ExprRes&& callable,
        ExprResList&& args);

    virtual ExprRes funcall_obj(
        Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args);

    virtual ExprRes do_funcall(
        Ref<ast::Node> node, Ref<Fun> fun, LValue self, ExprResList&& args);

    virtual ExprRes do_funcall_native(
        Ref<ast::Node> node, Ref<Fun> fun, LValue self, ExprResList&& args);

    virtual ExprRes empty_ret_val(Ref<ast::Node> node, Ref<Fun> fun);

    virtual std::pair<FunSet::Matches, ExprError> find_match(
        Ref<ast::Node> node,
        Ref<FunSet> fset,
        Ref<Class> dyn_cls,
        const TypedValueRefList& args);

    virtual ExprResList
    cast_args(Ref<ast::Node>, Ref<Fun> fun, ExprResList&& args);

    virtual ExprRes cast_arg(
        Ref<ast::Node> node,
        Ref<Fun> fun,
        Ref<Var> param,
        Ref<Type> to,
        ExprRes&& arg);
};

} // namespace ulam::sema

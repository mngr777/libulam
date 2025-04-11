#pragma once
#include <libulam/diag.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <utility>

namespace ulam::sema {

class EvalVisitor;

class EvalCast {
public:
    enum CastStatus { CastOk, InvalidCast, CastError, NoCast };
    using CastRes = std::pair<ExprRes, CastStatus>;

    EvalCast(EvalVisitor& eval, Diag& diag, Ref<Scope> scope):
        _eval{eval}, _diag{diag}, _scope{scope} {}

    virtual ExprRes
    cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl = false);

    virtual ExprRes cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl = false);

protected:
    virtual CastRes
    maybe_cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl);

    virtual CastRes maybe_cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes
    do_cast(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes do_cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes
    cast_class(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes cast_class_default(
        Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_class_fun(Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& arg, bool expl);

    virtual ExprRes cast_class_fun_after(
        Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_prim(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes cast_prim(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes
    cast_array(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_atom(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_ref(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_default(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    ExprRes take_ref(Ref<ast::Node> node, ExprRes&& arg);
    ExprRes deref(ExprRes&& arg);

    ExprRes change_type(Ref<Type> type, ExprRes&& arg);

    EvalVisitor& _eval;
    Diag& _diag;
    Ref<Scope> _scope;
};

} // namespace ulam::sema

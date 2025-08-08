#pragma once
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <utility>

namespace ulam::sema {

class EvalVisitor;

class EvalCast : public ScopedEvalHelper {
public:
    enum CastStatus {
        CastOk,
        CastError,
        NoCast
    };
    using CastRes = std::pair<ExprRes, CastStatus>;

    using ScopedEvalHelper::ScopedEvalHelper;

    virtual ExprRes
    cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl = false);

    virtual ExprRes cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl = false);

    virtual ExprRes cast_to_idx(Ref<ast::Node> node, ExprRes&& arg);

protected:
    virtual CastRes
    do_cast(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual CastRes do_cast(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes
    cast_prim(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_array(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_atom(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_class(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_ref(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes
    cast_default(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl);

    virtual ExprRes cast_default(
        Ref<ast::Node> node,
        BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl);

    virtual ExprRes cast_atom_to_nonelement_empty(
        Ref<ast::Node> node, Ref<Class> to, ExprRes&& arg);

    virtual ExprRes
    cast_class_fun(Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& arg, bool expl);

    virtual ExprRes take_ref(Ref<ast::Node> node, ExprRes&& arg);
    virtual ExprRes deref(ExprRes&& arg);

    Ref<Type> idx_type();
};

} // namespace ulam::sema

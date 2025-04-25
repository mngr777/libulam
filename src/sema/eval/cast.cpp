#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::sema {

ExprRes
EvalCast::cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl) {
    auto [res, _] = maybe_cast(node, type, std::move(arg), expl);
    return std::move(res);
}

ExprRes EvalCast::cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    auto [res, _] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    return std::move(res);
}

ExprRes EvalCast::cast_to_idx(Ref<ast::Node> node, ExprRes&& arg) {
    auto [res, _] = maybe_cast(node, idx_type(), std::move(arg), false);
    return std::move(res);
}

EvalCast::CastRes EvalCast::maybe_cast(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    CastStatus status = NoCast;
    assert(arg);

    bool deref_required = false;
    auto arg_type = arg.type();
    if (to->is_ref()) {
        // taking or casting a reference
        if (!arg_type->is_ref()) {
            status = CastRef;
            arg = take_ref(node, std::move(arg));
            arg_type = arg.type();
            if (!arg)
                return {std::move(arg), InvalidCast};
        }

    } else if (arg.value().is_lvalue()) {
        // copy value from reference
        // NOTE: actual dereferecing is postponed in case the argument is an
        // object and custom conversion is used: conversion function must be
        // called on the same object, see t3411
        if (arg_type->is_ref()) {
            status = CastDeref;
            arg_type = arg_type->deref();
        }
        deref_required = true;
    }

    if (to->is_same(arg_type)) {
        if (deref_required)
            arg = deref(std::move(arg));
        return {arg.derived(to, std::move(arg).move_value()), status};
    }

    // implicit cast to base class (TODO: non-ref)
    if (!expl && to->is_ref() && to->actual()->is_class() &&
        arg_type->actual()->is_class()) {
        auto to_cls = to->actual()->as_class();
        auto from_cls = arg.type()->actual()->as_class();
        if (to_cls->is_base_of(from_cls))
            return {arg.derived(to_cls, arg.move_value()), CastDowncast};
    }

    if (!expl && arg_type->is_impl_castable_to(to, arg.value())) {
        auto res = do_cast(node, to, std::move(arg), expl, deref_required);
        auto status = CastError;
        if (res)
            status = res.value().is_consteval() ? CastConsteval : CastOk;
        return {std::move(res), status};
    }

    if (arg_type->is_expl_castable_to(to)) {
        if (expl) {
            auto res = do_cast(node, to, std::move(arg), expl, deref_required);
            auto status = CastError;
            if (res)
                status = res.value().is_consteval() ? CastConsteval : CastOk;
            return {std::move(res), status};
        }
        diag().error(node, "suggest explicit cast");
    }
    diag().error(node, "invalid cast");
    return {ExprRes{ExprError::InvalidCast}, InvalidCast};
}

ExprRes EvalCast::do_cast(
    Ref<ast::Node> node,
    Ref<Type> to,
    ExprRes&& arg,
    bool expl,
    bool deref_required) {
    auto from = arg.type();
    if (deref_required)
        from = arg.type()->deref();

    assert(!to->is_ref() || (from->is_ref() && !arg.value().is_tmp()));
    assert(to->is_ref() || !from->is_ref());
    assert(!to->is_same(from));
    assert(from->is_expl_castable_to(to));

    // dereference non-object value
    if (deref_required && !from->is_class())
        arg = deref(std::move(arg));

    if (from->is_prim()) {
        return cast_prim(node, to, std::move(arg), expl);

    } else if (from->is_class()) {
        return cast_class(node, to, std::move(arg), expl, deref_required);

    } else if (from->is_array()) {
        assert(to->is_array());
        return cast_array(node, to, std::move(arg), expl);

    } else if (from->is(AtomId)) {
        return cast_atom(node, to, std::move(arg), expl);

    } else if (from->is_ref()) {
        assert(to->is_ref());
        return cast_ref(node, to, std::move(arg), expl);
    }
    assert(false);
}

EvalCast::CastRes EvalCast::maybe_cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    assert(arg);
    if (arg.type()->actual()->is(bi_type_id))
        return {std::move(arg), NoCast};

    auto res = do_cast(node, bi_type_id, std::move(arg), expl);
    auto status = res ? CastOk : CastError;
    return {std::move(res), status};
}

ExprRes EvalCast::do_cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    if (arg.value().is_lvalue())
        arg = deref(std::move(arg));

    if (arg.type()->is_prim()) {
        return cast_prim(node, bi_type_id, std::move(arg), expl);

    } else if (arg.type()->is_class()) {
        // class conversion
        auto cls = arg.type()->as_class();
        auto convs = cls->convs(bi_type_id, true);
        assert(convs.size() == 1);

        arg = cast_class_fun(node, *convs.begin(), std::move(arg), expl);

        assert(arg.type()->is_prim());
        if (!arg.type()->is(bi_type_id))
            arg = do_cast(node, bi_type_id, std::move(arg), expl);
        return std::move(arg);
    }
    assert(false);
}

ExprRes EvalCast::cast_class(
    Ref<ast::Node> node,
    Ref<Type> to,
    ExprRes&& arg,
    bool expl,
    bool deref_required) {
    assert(arg.type()->deref()->is_class());
    auto cls = arg.type()->deref()->as_class();
    auto convs = cls->convs(to, true);
    if (convs.size() == 0) {
        assert(cls->is_castable_to(to, true));
        if (deref_required) {
            // dereference object value
            arg = deref(std::move(arg));
        }
        return cast_class_default(node, to, std::move(arg), expl);

    } else {
        arg = cast_class_fun(node, *convs.begin(), std::move(arg), expl);
        if (!arg.type()->is_same(to)) {
            assert(arg.type()->is_expl_castable_to(to));
            return cast_class_fun_after(node, to, std::move(arg), expl);
        }
        return std::move(arg);
    }
}

ExprRes EvalCast::cast_class_default(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_class());
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_class_fun(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_class());
    auto funcall = eval().funcall_helper(scope(), flags());
    ExprRes res = funcall->funcall(node, fun, std::move(arg), {});
    if (!res)
        diag().error(node, "conversion failed");
    return res;
}

ExprRes EvalCast::cast_class_fun_after(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    return do_cast(node, to, std::move(arg), expl, false);
}

ExprRes EvalCast::cast_prim(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_prim());
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_prim(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_prim());
    auto tv = arg.type()->as_prim()->cast_to(bi_type_id, arg.move_value());
    return arg.derived(std::move(tv));
}

ExprRes EvalCast::cast_array(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_array());
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_atom(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is(AtomId));
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_ref(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_ref());
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_default(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    return arg.derived({to, arg.type()->cast_to(to, arg.move_value())});
}

ExprRes EvalCast::take_ref(Ref<ast::Node> node, ExprRes&& arg) {
    auto type = arg.type();
    const auto& val = arg.value();

    if (val.is_rvalue()) {
        diag().error(node, "cannot take a reference of rvalue");
        return {ExprError::InvalidCast};
    }
    if (val.lvalue().is_xvalue()) {
        diag().error(node, "cannot take a reference to xvalue");
        return {ExprError::InvalidCast};
    }
    if (type->deref()->is_class() && val.has_rvalue())
        type = val.dyn_obj_type()->ref_type();
    type = type->ref_type();

    return arg.derived(type, arg.move_value());
}

ExprRes EvalCast::deref(ExprRes&& arg) {
    assert(arg.value().is_lvalue());
    auto type = arg.type();
    auto val = arg.move_value();
    val = val.deref();
    type = type->deref();
    if (arg.type()->is_class() && arg.value().has_rvalue())
        type = val.dyn_obj_type();

    return arg.derived(type, std::move(val));
}

Ref<Type> EvalCast::idx_type() { return builtins().int_type(); }

} // namespace ulam::sema

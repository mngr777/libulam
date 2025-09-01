#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::sema {

using CastRes = EvalCast::CastRes;

ExprRes
EvalCast::cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl) {
    auto [res, _] = do_cast(node, type, std::move(arg), expl);
    return std::move(res);
}

ExprRes EvalCast::cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    auto [res, _] = do_cast(node, bi_type_id, std::move(arg), expl);
    return std::move(res);
}

CastRes
EvalCast::do_cast(Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    if (!arg)
        return {std::move(arg), CastError};

    auto from = arg.type();
    if (from->is_same(to)) {
        if (!to->is_ref() && !has_flag(evl::NoDerefCast) &&
            arg.value().is_lvalue()) {
            arg = deref(std::move(arg));
            return {arg.derived(to, arg.move_value()), CastOk};
        }
        return {std::move(arg), NoCast};
    }

    // NOTE: class conversions must be called on same object, so class/atom refs
    // are handled in `cast_class` and `cast_atom`
    if (from->is_prim()) {
        arg = cast_prim(node, to, std::move(arg), expl);

    } else if (from->is_array()) {
        arg = cast_array(node, to, std::move(arg), expl);

    } else if (from->deref()->is(AtomId)) {
        arg = cast_atom(node, to, std::move(arg), expl);

    } else if (from->deref()->is_class()) {
        arg = cast_class(node, to, std::move(arg), expl);

    } else if (from->is_ref()) {
        arg = cast_ref(node, to, std::move(arg), expl);

    } else {
        assert(false);
    }

    auto status = arg.ok() ? CastOk : CastError;
    return {std::move(arg), status};
}

CastRes EvalCast::do_cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    auto from = arg.type();
    if (from->is(bi_type_id))
        return {std::move(arg), NoCast};

    if (from->deref()->is_prim()) {
        if (arg.type()->is_ref())
            arg = deref(std::move(arg));
        arg = cast_default(node, bi_type_id, std::move(arg), expl);

    } else if (from->deref()->is_class()) {
        // class conversion
        auto cls = from->deref()->as_class();
        auto convs = cls->convs(bi_type_id, true);
        assert(convs.size() == 1);

        arg = cast_class_fun(node, *convs.begin(), std::move(arg), expl);
        assert(arg.type()->is_prim());
        if (!arg.type()->is(bi_type_id))
            arg = cast_default(node, bi_type_id, std::move(arg), expl);

    } else {
        assert(false);
    }

    auto status = arg.ok() ? CastOk : CastError;
    return {std::move(arg), status};
}

ExprRes EvalCast::cast_prim(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_prim());
    if (to->is_ref()) {
        arg = take_ref(node, std::move(arg));
        if (!arg || arg.type()->is_same(to))
            return std::move(arg);
    }
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_array(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_array());
    assert(!arg.type()->is_same(to));

    if (to->is_ref()) {
        arg = take_ref(node, std::move(arg));
        if (!arg || arg.type()->is_same(to))
            return std::move(arg);
    }
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_atom(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->deref()->is(AtomId));
    assert(!arg.type()->is_same(to));

    if (arg.type()->is_ref() != to->is_ref()) {
        arg = arg.type()->is_ref() ? deref(std::move(arg))
                                   : take_ref(node, std::move(arg));
        if (arg.type()->is_same(to))
            return std::move(arg);

        if (arg.type()->is_class())
            return cast_class(node, to, std::move(arg), expl);
    }
    if (arg.type()->is_ref())
        return cast_default(node, to, std::move(arg), expl);
    assert(arg.type()->is(AtomId));

    if (!to->is_class()) {
        return {ExprError::InvalidCast};
    }

    auto cls = to->as_class();
    if (!cls->is_element()) {
        // value is unknown, assume dynamic type is element derived from arg
        // class
        if (arg.value().empty())
            return cast_atom_to_nonelement_empty(node, cls, std::move(arg));
    }
    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_class(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->deref()->is_class());
    auto cls = arg.type()->deref()->as_class();

    auto convs = cls->convs(to, true);
    if (convs.size() == 0) {
        if (arg.type()->is_ref() != to->is_ref()) {
            arg = arg.type()->is_ref() ? deref(std::move(arg))
                                       : take_ref(node, std::move(arg));
        }
        if (arg.type()->is_same(to))
            return std::move(arg);

        return cast_default(node, to, std::move(arg), expl);

    } else {
        arg = cast_class_fun(node, *convs.begin(), std::move(arg), expl);
        if (!arg.type()->is_same(to)) {
            assert(arg.type()->is_castable_to(to, arg.value(), expl));
            arg = cast_default(node, to, std::move(arg), expl);
        }
        return std::move(arg);
    }
}

ExprRes EvalCast::cast_ref(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    assert(arg.type()->is_ref());
    assert(!arg.type()->deref()->is_class());
    assert(!arg.type()->is_same(to));

    if (!to->is_ref()) {
        arg = deref(std::move(arg));
        assert(arg);
        if (arg.type()->is_same(to))
            return std::move(arg);
    }

    return cast_default(node, to, std::move(arg), expl);
}

ExprRes EvalCast::cast_default(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    if (!arg)
        return std::move(arg);
    if (!arg.type()->is_castable_to(to, arg.value(), expl)) {
        auto message = std::string{"invalid cast from "} + arg.type()->name() +
                       " to " + to->name();
        diag().error(node, message);
        return {ExprError::InvalidCast};
    }
    return arg.derived({to, arg.type()->cast_to(to, arg.move_value())});
}

ExprRes EvalCast::cast_default(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    if (!arg)
        return std::move(arg);
    if (!arg.type()->is_prim() ||
        !arg.type()->is_castable_to(bi_type_id, arg.value(), expl)) {
        auto message = std::string{"invalid cast from "} + arg.type()->name() +
                       " to " + std::string{builtin_type_str(bi_type_id)};
        diag().error(node, message);
        return {ExprError::InvalidCast};
    }
    auto prim_type = arg.type()->as_prim();
    return arg.derived(prim_type->cast_to(bi_type_id, arg.move_value()));
}

ExprRes EvalCast::cast_atom_to_nonelement_empty(
    Ref<ast::Node> node, Ref<Class> to, ExprRes&& arg) {
    assert(arg.value().empty());
    assert(to->is_quark());
    auto val = arg.value().is_lvalue() ? Value{arg.value().lvalue().derived()}
                                       : Value{RValue{}};
    return {to, std::move(val)};
}

ExprRes EvalCast::cast_class_fun(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& arg, bool expl) {
    assert(arg.type()->deref()->is_class());
    auto res = env().funcall(node, fun, std::move(arg), {});
    if (!res)
        diag().error(node, "conversion failed");
    return res;
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

    return arg.derived(type->ref_type(), arg.move_value());
}

ExprRes EvalCast::deref(ExprRes&& arg) {
    assert(arg.value().is_lvalue());
    Value val = arg.move_value().deref();
    if (!arg.type()->deref()->is_object() || val.empty())
        return arg.derived(arg.type()->deref(), std::move(val));
    return arg.derived(val.dyn_obj_type(), std::move(val));
}

} // namespace ulam::sema

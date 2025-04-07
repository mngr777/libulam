#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::sema {

ExprRes EvalCast::cast(
    Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl) {
    auto [res, _] = maybe_cast(node, type, std::move(arg), expl);
    return std::move(res);
}

ExprRes EvalCast::cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    auto [res, _] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    return std::move(res);
}

EvalCast::CastRes EvalCast::maybe_cast(
    Ref<ast::Node> node, Ref<Type> to, ExprRes&& arg, bool expl) {
    auto tv = arg.move_typed_value();
    auto from = tv.type();
    auto val = tv.move_value();

    if (to->is_ref()) {
        // taking or casting a reference
        if (val.is_rvalue()) {
            _diag.error(node, "cannot take a reference of rvalue");
            return {ExprRes{ExprError::InvalidCast}, InvalidCast};
        }
        if (val.lvalue().is_xvalue()) {
            _diag.error(node, "cannot take a reference to xvalue");
            return {ExprRes{ExprError::InvalidCast}, InvalidCast};
        }
        if (from->deref()->is_class())
            from = val.dyn_obj_type()->ref_type();
        from = from->ref_type();

    } else if (from->is_ref() || val.is_lvalue()) {
        // copy value from reference
        assert(val.is_lvalue());
        val = val.deref();
        from = from->deref();
        if (from->is_class())
            from = val.dyn_obj_type();
    }

    if (to->is_same(from))
        return {ExprRes{to, std::move(val)}, NoCast};

    // implicit cast to base class
    if (!expl && to->is_ref() && to->actual()->is_class() &&
        from->actual()->is_class()) {
        auto to_cls = to->actual()->as_class();
        auto from_cls = from->actual()->as_class();
        if (to_cls->is_base_of(from_cls))
            return {ExprRes{to, std::move(val)}, NoCast};
    }

    if (!expl && from->is_impl_castable_to(to, val)) {
        auto res = do_cast(node, to, {from, std::move(val)});
        auto status = res ? CastOk : CastError;
        return {std::move(res), status};
    }

    if (from->is_expl_castable_to(to)) {
        if (expl) {
            auto res = do_cast(node, to, {from, std::move(val)});
            auto status = res ? CastOk : CastError;
            return {std::move(res), status};
        }
        _diag.error(node, "suggest explicit cast");
    }
    _diag.error(node, "invalid cast");
    return {ExprRes{ExprError::InvalidCast}, InvalidCast};
}

ExprRes EvalCast::do_cast(Ref<ast::Node> node, Ref<Type> to, TypedValue&& tv) {
    auto from = tv.type();
    auto val = tv.move_value();

    if (to->is_ref()) {
        // taking or casting a reference
        assert(!val.is_tmp());
        from = from->ref_type();

    } else if (from->is_ref()) {
        // copy value from reference
        assert(val.is_lvalue());
        from = from->deref();
        val = val.deref();
    }

    assert(!to->is_same(from));
    assert(from->is_expl_castable_to(to));

    if (from->is_prim()) {
        return {to, from->as_prim()->cast_to(to, std::move(val))};

    } else if (from->is_class()) {
        auto cls = from->as_class();
        auto convs = cls->convs(to, true);
        if (convs.size() == 0) {
            assert(cls->is_castable_to(to, true));
            return {to, cls->cast_to(to, std::move(val))};

        } else {
            auto funcall = _eval.funcall_helper(_scope);
            ExprRes res =
                funcall->funcall(node, *convs.begin(), val.self(), {});
            if (!res) {
                _diag.error(node, "conversion failed");
                return res;
            }
            if (!res.type()->is_same(to)) {
                assert(res.type()->is_expl_castable_to(to));
                return do_cast(node, to, res.move_typed_value());
            }
            return res;
        }
    } else if (from->is_array()) {
        assert(to->is_array());
        return {to, from->cast_to(to, std::move(val))};

    } else if (from->is(AtomId)) {
        return {to, from->cast_to(to, std::move(val))};

    } else if (from->is_ref()) {
        assert(to->is_ref());
        return {to, from->cast_to(to, std::move(val))};
    }

    assert(false);
}

EvalCast::CastRes EvalCast::maybe_cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    auto tv = arg.move_typed_value();
    if (tv.type()->actual()->is(bi_type_id))
        return {ExprRes{std::move(tv)}, NoCast};

    auto res = do_cast(node, bi_type_id, std::move(tv));
    auto status = res ? CastOk : CastError;
    return {std::move(res), status};
}

ExprRes EvalCast::do_cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, TypedValue&& tv) {
    auto type = tv.type()->actual();

    if (type->is_class()) {
        // class conversion
        auto cls = type->as_class();
        auto convs = cls->convs(bi_type_id, true);
        assert(convs.size() == 1);
        auto self = tv.move_value().self();
        auto funcall = _eval.funcall_helper(_scope);
        ExprRes res = funcall->funcall(node, *convs.begin(), self, {});
        if (!res) {
            _diag.error(node, "conversion failed");
            return res;
        }
        assert(res.type()->is_prim());
        if (!res.type()->is(bi_type_id))
            res = do_cast(node, bi_type_id, res.move_typed_value());
        return res;
    }

    // primitive to builtin
    auto val = tv.move_value().deref();
    if (type->is_prim()) {
        return {type->as_prim()->cast_to(bi_type_id, std::move(val))};
    }
    assert(false);
}

} // namespace ulam::sema

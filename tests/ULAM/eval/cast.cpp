#include "./cast.hpp"
#include "./expr_flags.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalCast] "
#endif
#include "src/debug.hpp"

namespace {

using ExprRes = ulam::sema::ExprRes;

void unset_internal_res_flags(ExprRes& res) {
    res.uns_flag(exp::RefCastInternal);
    res.uns_flag(exp::OmitCastInternal);
}

} // namespace

ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ExprRes&& arg,
    bool expl) {
    auto res = Base::cast(node, type, std::move(arg), expl);
    if (!has_flag(evl::NoCodegen))
        unset_internal_res_flags(res);
    return res;
}

ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    auto res = Base::cast(node, bi_type_id, std::move(arg), expl);
    if (!has_flag(evl::NoCodegen))
        unset_internal_res_flags(res);
    return res;
}

ExprRes EvalCast::cast_to_idx(ulam::Ref<ulam::ast::Node> node, ExprRes&& arg) {
    auto no_consteval_cast = flags_raii(flags() | evl::NoConstevalCast);
    auto type = idx_type();
    if (!has_flag(evl::NoCodegen)) {
        auto arg_type = arg.type()->deref();
        if ((arg_type->is(ulam::UnsignedId) || arg.type()->is(ulam::IntId)) &&
            arg_type->is_impl_castable_to(type))
            arg.set_flag(exp::OmitCastInternal);
    }
    auto [res, _] = do_cast(node, idx_type(), std::move(arg), false);
    if (!has_flag(evl::NoCodegen))
        unset_internal_res_flags(res);
    return std::move(res);
}

ExprRes EvalCast::cast_atom_to_nonelement_empty(
    ulam::Ref<ulam::ast::Node> node, ulam::Ref<ulam::Class> to, ExprRes&& arg) {
    std::string data;
    if (!has_flag(evl::NoCodegen))
        data = exp::data(arg);
    auto res = Base::cast_atom_to_nonelement_empty(node, to, std::move(arg));
    if (!data.empty())
        exp::set_data(res, data);
    return res;
}

ExprRes EvalCast::cast_class_fun(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ExprRes&& arg,
    bool expl) {
    auto res = Base::cast_class_fun(node, fun, std::move(arg), expl);
    // no need to add "cast" unless second cast is required (or cast is
    // explicit)
    if (!has_flag(evl::NoCodegen) && !expl)
        res.set_flag(exp::OmitCastInternal);
    return res;
}

ExprRes EvalCast::cast_default(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ExprRes&& arg,
    bool expl) {
    auto res = Base::cast_default(node, to, std::move(arg), expl);
    update_res(res, expl);
    return res;
}

ExprRes EvalCast::cast_default(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    auto res = Base::cast_default(node, bi_type_id, std::move(arg), expl);
    update_res(res, expl);
    return res;
}

ExprRes EvalCast::take_ref(ulam::Ref<ulam::ast::Node> node, ExprRes&& arg) {
    bool is_ref = arg.type()->is_ref();
    auto res = Base::take_ref(node, std::move(arg));
    if (!is_ref) {
        update_res(res, false);
        res.set_flag(exp::RefCastInternal);
    }
    return res;
}

ExprRes EvalCast::deref(ExprRes&& arg) {
    bool is_ref = arg.type()->is_ref();
    auto res = Base::deref(std::move(arg));
    if (is_ref) {
        update_res(res, false);
        res.set_flag(exp::RefCastInternal);
    }
    return res;
}

void EvalCast::update_res(ExprRes& res, bool expl) {
    bool omit_cast = res.has_flag(exp::OmitCastInternal);
    res.uns_flag(exp::OmitCastInternal);

    if (!res || has_flag(evl::NoCodegen) || omit_cast)
        return;

    auto data = exp::data(res);
    bool is_consteval =
        res.value().is_consteval() && has_flag(evl::NoConstevalCast);

    if (expl || !is_consteval) {
        if (!res.has_flag(exp::RefCastInternal)) {
            exp::add_cast(res, expl);
        } else {
            res.uns_flag(exp::RefCastInternal);
            res.uns_flag(exp::ImplCast);
            res.uns_flag(exp::ExplCast);
            res.set_flag(expl ? exp::ExplCast : exp::ImplCast);
        }

    } else if (is_consteval) {
        assert(res.value().is_consteval());
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            Stringifier stringifier{program()};
            stringifier.options.unary_as_unsigned_lit = true;
            stringifier.options.bool_as_unsigned_lit = true;
            exp::set_data(res, stringifier.stringify(res.type(), rval));
        });
    }
}

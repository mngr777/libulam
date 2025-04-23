#include "./cast.hpp"
#include "./expr_flags.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalCast] "
#endif
#include "src/debug.hpp"

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    arg.uns_flag(exp::OmitCast);
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    arg.uns_flag(exp::OmitCast);
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast_class_default(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto type = arg.type();
    bool is_consteval = arg.value().is_consteval();
    bool is_element = type->is_class() && type->as_class()->is_element();

    auto res = Base::cast_class_default(node, to, std::move(arg), expl);
    if (!expl && is_consteval && is_element && to->is(ulam::AtomId))
        res.set_flag(exp::OmitCast);
    return res;
}

ulam::sema::ExprRes EvalCast::cast_class_fun(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto res = Base::cast_class_fun(node, fun, std::move(arg), expl);
    // no need to add "cast" unless second cast is required (or cast is
    // explicit)
    res.set_flag(exp::OmitCast);
    return res;
}

ulam::sema::ExprRes EvalCast::cast_class_fun_after(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    // needs a second cast after conversion
    arg.uns_flag(exp::OmitCast);
    return Base::cast_class_fun_after(node, to, std::move(arg), expl);
}

void EvalCast::update_res(
    ulam::sema::ExprRes& res, EvalCast::CastStatus status, bool expl) {
    if (!res || (!expl && status == EvalCast::NoCast))
        return;
    assert(status != EvalCast::InvalidCast && status != EvalCast::CastError);

    auto data = exp::data(res);
    bool is_conv = res.has_flag(exp::OmitCast);
    bool is_consteval =
        status == EvalCast::CastConsteval && (flags() & evl::NoConstevalCast);
    res.uns_flag(exp::OmitCast);

    if (expl || (!is_conv && !is_consteval))
        exp::add_cast(res, expl);
}

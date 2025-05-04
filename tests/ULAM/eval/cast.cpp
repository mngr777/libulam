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

}

ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ExprRes&& arg,
    bool expl) {
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
}

ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
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
    auto [res, status] = maybe_cast(node, idx_type(), std::move(arg), false);
    update_res(res, status, false);
    return std::move(res);
}

ExprRes EvalCast::cast_atom_to_quark_noexec(
    ulam::Ref<ulam::ast::Node> node, ulam::Ref<ulam::Class> to, ExprRes&& arg) {
    std::string data;
    if (!has_flag(evl::NoCodegen))
        data = exp::data(arg);
    auto res = Base::cast_atom_to_quark_noexec(node, to, std::move(arg));
    if (!data.empty())
        exp::set_data(res, data);
    return res;
}

ExprRes EvalCast::cast_class_default(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ExprRes&& arg,
    bool expl) {
    auto type = arg.type();
    bool is_consteval = arg.value().is_consteval();
    bool is_element = type->is_class() && type->as_class()->is_element();

    auto res = Base::cast_class_default(node, to, std::move(arg), expl);
    // omit cast from consteval element to Atom, t3416
    if (!has_flag(evl::NoCodegen)) {
        if (!expl && is_consteval && is_element && to->is(ulam::AtomId))
            res.set_flag(exp::OmitCastInternal);
    }
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

ExprRes EvalCast::cast_class_fun_after(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ExprRes&& arg,
    bool expl) {
    // needs a second cast after conversion
    arg.uns_flag(exp::OmitCastInternal);
    return Base::cast_class_fun_after(node, to, std::move(arg), expl);
}

void EvalCast::update_res(
    ExprRes& res, EvalCast::CastStatus status, bool expl) {
    bool omit_cast = res.has_flag(exp::OmitCastInternal);
    res.uns_flag(exp::OmitCastInternal);

    if (has_flag(evl::NoCodegen))
        return;
    if (!res)
        return;
    if (omit_cast || (!expl && status == EvalCast::NoCast))
        return;
    assert(status != EvalCast::InvalidCast && status != EvalCast::CastError);

    auto data = exp::data(res);
    bool is_consteval =
        status == EvalCast::CastConsteval && has_flag(evl::NoConstevalCast);

    if (expl || !is_consteval) {
        exp::add_cast(res, expl);

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

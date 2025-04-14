#include "./cast.hpp"
#include "./expr_flags.hpp"
#include "libulam/sema/eval/cast.hpp"
#include <cassert>

namespace {

void update_res(
    ulam::sema::ExprRes& res, EvalCast::CastStatus status, bool expl) {
    if (!res || status == EvalCast::NoCast)
        return;
    assert(status != EvalCast::InvalidCast && status != EvalCast::CastError);
    auto data = res.data<std::string>("");
    if (!data.empty()) {
        if (expl || !res.has_flag(exp::ConvCast))
            res.set_data(data + " cast");
    } else {
        res.uns_data();
    }
    res.uns_flag(exp::ConvCast);
    res.set_flag(expl ? exp::ExplCast : exp::ImplCast);
}

} // namespace

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    arg.uns_flag(exp::ConvCast);
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    arg.uns_flag(exp::ConvCast);
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    update_res(res, status, expl);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast_class_fun(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto res = ulam::sema::EvalCast::cast_class_fun(
        node, fun, std::move(arg), expl);
    // no need to add "cast" unless second cast is required (or cast is explicit)
    res.set_flag(exp::ConvCast);
    return res;
}

ulam::sema::ExprRes EvalCast::cast_class_fun_after(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    // needs a second cast after conversion
    arg.uns_flag(exp::ConvCast);
    return ulam::sema::EvalCast::cast_class_fun_after(
        node, to, std::move(arg), expl);
}

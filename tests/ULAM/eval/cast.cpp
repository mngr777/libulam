#include "./cast.hpp"
#include "./expr_flags.hpp"
#include <cassert>

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    if (status == CastOk)
        res.set_flag(expl ? ExplCast : ImplCast);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    if (status == CastOk)
        res.set_flag(expl ? ExplCast : ImplCast);
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast_prim(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg) {
    auto data = arg.data<std::string>("");
    auto res =
        ulam::sema::EvalCast::cast_prim(node, bi_type_id, std::move(arg));
    if (!data.empty()) {
        res.set_data(data + " cast");
    } else {
        res.uns_data();
    }
    return res;
}

ulam::sema::ExprRes EvalCast::cast_default(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> to,
    ulam::sema::ExprRes&& arg) {
    auto data = arg.data<std::string>("");
    auto res = ulam::sema::EvalCast::cast_default(node, to, std::move(arg));
    if (!data.empty()) {
        res.set_data(data + " cast");
    } else {
        res.uns_data();
    }
    return res;
}

#include "./cast.hpp"
#include <cassert>

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto data = arg.data<std::string>("");
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    if (!data.empty()) {
        if (status == CastOk)
            data += " cast";
        res.set_data(data);
    }
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto data = arg.data<std::string>("");
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    if (!data.empty()) {
        if (status == CastOk)
            data += " cast";
        res.set_data(data);
    }
    return std::move(res);
}

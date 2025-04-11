#include "./cast.hpp"
#include "./expr_flags.hpp"
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <cassert>

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto arg_type = arg.type();
    auto data = arg.data<std::string>("");
    auto [res, status] = maybe_cast(node, type, std::move(arg), expl);
    if (!data.empty()) {
        if (status == CastOk) {
            if (arg_type->is_class() && type->is_prim()) {
                data += " ( )toInt .";
            } else {
                data += " cast";
            }
            res.set_flag(expl ? ExplCast : ImplCast);
        }
        res.set_data(data);
    }
    return std::move(res);
}

ulam::sema::ExprRes EvalCast::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ulam::sema::ExprRes&& arg,
    bool expl) {
    auto arg_type = arg.type();
    auto data = arg.data<std::string>("");
    auto [res, status] = maybe_cast(node, bi_type_id, std::move(arg), expl);
    if (!data.empty()) {
        if (status == CastOk) {
            if (arg_type->is_class() && ulam::is_prim(bi_type_id)) {
                data += " ( )toInt .";
            } else {
                data += " cast";
            }
            res.set_flag(expl ? ExplCast : ImplCast);
        }
        res.set_data(data);
    }
    return std::move(res);
}

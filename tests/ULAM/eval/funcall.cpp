#include "./funcall.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include <libulam/sema/eval/flags.hpp>
#include <string>

namespace {

using ExprRes = EvalFuncall::ExprRes;
using ExprResList = EvalFuncall::ExprResList;

void replace(
    std::string& data, const std::string_view ph, const std::string_view repl) {
    auto pos = data.rfind(ph);
    assert(pos != std::string::npos);
    data.replace(pos, ph.size(), repl);
}

} // namespace

ExprRes EvalFuncall::construct_funcall(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::Fun> fun,
    ulam::RValue&& rval,
    ExprResList&& args) {
    std::string data;
    if (!has_flag(evl::NoCodegen))
        data = arg_data(args) + "Self";
    auto res = Base::construct_funcall(
        node, cls, fun, std::move(rval), std::move(args));
    if (!data.empty())
        exp::set_data(res, data);
    return res;
}

ExprRes EvalFuncall::funcall_callable(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ExprRes&& callable,
    ExprResList&& args) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(callable);
        replace(data, "{args}", arg_data(args));
        replace(data, "{fun}", str(fun->name_id()));
    }
    auto res =
        Base::funcall_callable(node, fun, std::move(callable), std::move(args));
    if (!data.empty())
        exp::set_data(res, data);
    return res;
}

ExprRes EvalFuncall::funcall_obj(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ExprRes&& obj,
    ExprResList&& args) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        auto call_data = arg_data(args) + std::string{str(fun->name_id())};
        data = exp::data_combine(data, call_data, ".");
    }
    auto res = Base::funcall_obj(node, fun, std::move(obj), std::move(args));
    if (!data.empty())
        res.set_data(data);
    return res;
}

ExprResList EvalFuncall::cast_args(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ExprResList&& args) {
    // do not omit consteval casts for arguments (t3233)
    auto consteval_cast_raii = flags_raii(flags() & ~evl::NoConstevalCast);
    return Base::cast_args(node, fun, std::move(args));
}

ExprRes EvalFuncall::cast_arg(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::Ref<ulam::Var> param,
    ulam::Ref<ulam::Type> to,
    ExprRes&& arg) {
    arg = Base::cast_arg(node, fun, param, to, std::move(arg));
    if (!has_flag(evl::NoCodegen)) {
        auto data = exp::data(arg);
        if (!arg.type()->is_same(param->type()) ||
            (to->is_ref() && param->is_const() && !arg.value().is_consteval()))
            exp::add_cast(arg);
    }
    return std::move(arg);
}

std::string EvalFuncall::arg_data(const ExprResList& args) {
    if (args.empty())
        return "( )";

    std::string data = "(";
    for (const auto& arg : args)
        data = exp::data_append(data, exp::data(arg));
    return exp::data_append(data, ")");
}

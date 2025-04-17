#include "./funcall.hpp"
#include "./expr_res.hpp"
#include "tests/ULAM/eval/flags.hpp"
#include <string>

namespace {

void replace(
    std::string& data, const std::string_view ph, const std::string_view repl) {
    auto pos = data.rfind(ph);
    assert(pos != std::string::npos);
    data.replace(pos, ph.size(), repl);
}

} // namespace

ulam::sema::ExprRes EvalFuncall::funcall_callable(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& callable,
    ulam::sema::ExprResList&& args) {

    auto data = exp::data(callable);
    replace(data, "{args}", arg_data(args));

    if (!fun->is_op() || fun->is_op_alias()) {
        replace(data, "{fun}", str(fun->name_id()));
    } else {
        // TODO: operators
    }

    auto res =
        Base::funcall_callable(node, fun, std::move(callable), std::move(args));

    exp::set_data(res, data);
    return res;
}

ulam::sema::ExprRes EvalFuncall::funcall_obj(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& obj,
    ulam::sema::ExprResList&& args) {
    assert(!fun->is_op() || fun->is_op_alias());

    auto data = exp::data(obj);
    auto call_data = arg_data(args) + std::string{str(fun->name_id())};
    data = exp::data_combine(data, call_data, ".");

    auto res = Base::funcall_obj(node, fun, std::move(obj), std::move(args));

    res.set_data(data);
    return res;
}

ulam::sema::ExprResList EvalFuncall::cast_args(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprResList&& args) {
    // do not omit consteval casts for arguments (t3233)
    auto consteval_cast_raii = flags_raii(flags() & ~evl::NoConstevalCast);
    return Base::cast_args(node, fun, std::move(args));
}

std::string EvalFuncall::arg_data(const ulam::sema::ExprResList& args) {
    if (args.empty())
        return "( )";

    std::string data = "(";
    for (const auto& arg : args)
        data = exp::data_append(data, exp::data(arg));
    return exp::data_append(data, ")");
}

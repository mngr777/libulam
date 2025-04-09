#include "./funcall.hpp"
#include <string>

ulam::sema::ExprRes EvalFuncall::do_funcall(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& callable,
    ulam::sema::ExprResList&& args) {

    std::string data{};
    if (callable.has_data()) {
        std::string arg_data{};
        for (const auto& arg : args) {
            if (!arg_data.empty())
                arg_data += " ";
            if (!arg.has_data()) {
                arg_data.clear();
                break;
            }
            arg_data += arg.data<std::string>();
        }
        if (!arg_data.empty()) {
            arg_data = "( " + arg_data + " )";
            data = callable.data<std::string>();
            auto pos = data.find("{args}");
            if (pos != std::string::npos) {
                data.replace(pos, std::string_view{"{args}"}.size(), arg_data);
            } else {
                data.clear();
            }
        }
    }

    auto res = ulam::sema::EvalFuncall::do_funcall(
        node, fun, std::move(callable), std::move(args));
    if (!data.empty()) {
        res.set_data(data);
    } else {
        res.uns_data();
    }
    return res;
}

#include "./funcall.hpp"
#include "libulam/sema/eval/funcall.hpp"
#include <string>

ulam::sema::ExprRes EvalFuncall::funcall_callable(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& callable,
    ulam::sema::ExprResList&& args) {
    static const std::string_view FunPh{"{fun}"};
    static const std::string_view ArgsPh{"{args}"};

    std::string data;
    if (callable.has_data()) {
        auto arg_str = arg_data(args);
        if (!arg_str.empty()) {
            data = callable.data<std::string>();
            auto pos = data.rfind(ArgsPh);
            if (pos != std::string::npos) {
                data.replace(pos, ArgsPh.size(), arg_str);
            } else {
                data.clear();
            }
        }

        if (!fun->is_op() || fun->is_op_alias()) {
            std::string name{_str_pool.get(fun->name_id())};
            auto pos = data.rfind(FunPh);
            if (pos != std::string::npos) {
                data.replace(pos, FunPh.size(), name);
            } else {
                data.clear();
            }
        } else {
            // TODO: operators
        }
    }

    auto res = ulam::sema::EvalFuncall::funcall_callable(
        node, fun, std::move(callable), std::move(args));
    if (!data.empty()) {
        res.set_data(data);
    } else {
        res.uns_data();
    }
    return res;
}

ulam::sema::ExprRes EvalFuncall::funcall_obj(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::sema::ExprRes&& obj,
    ulam::sema::ExprResList&& args) {

    std::string data;
    if (obj.has_data()) {
        auto arg_str = arg_data(args);
        if (!arg_str.empty()) {
            std::string fun_name{_str_pool.get(fun->name_id())};
            data = obj.data<std::string>() + " " + arg_str + fun_name + " .";
        }
    }

    auto res = ulam::sema::EvalFuncall::funcall_obj(
        node, fun, std::move(obj), std::move(args));
    if (!data.empty()) {
        res.set_data(data);
    } else {
        res.uns_data();
    }
    return res;
}

std::string EvalFuncall::arg_data(const ulam::sema::ExprResList& args) {
    std::string data;
    for (const auto& arg : args) {
        if (!arg.has_data()) {
            data.clear();
            break;
        }
        data += arg.data<std::string>() + " ";
    }
    if (!args.empty() && data.empty())
        return data;
    return "( " + data + ")";
}

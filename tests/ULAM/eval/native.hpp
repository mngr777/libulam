#pragma once
#include "./helper.hpp"
#include <functional>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/value.hpp>
#include <map>
#include <ostream>
#include <string_view>
#include <utility>

class EvalNative : public EvalHelper, public ulam::sema::EvalHelper {
public:
    using Base = ulam::sema::EvalHelper;
    using ExprRes = ulam::sema::ExprRes;
    using ExprResList = ulam::sema::ExprResList;

    explicit EvalNative(EvalEnv& env);

    ExprRes call(
        ulam::Ref<ulam::ast::Node> node,
        const std::string_view class_name,
        const std::string_view fun_name,
        ulam::LValue self,
        ExprResList&& args);

private:
    using NamePair = std::pair<const std::string_view, const std::string_view>;
    using FunImpl = std::function<ExprRes(ulam::LValue, ExprResList&&)>;
    using Map = std::map<NamePair, FunImpl>;

    // System
    ExprRes eval_system_print_int(ulam::LValue self, ExprResList&& args);
    ExprRes eval_system_print_unsigned(ulam::LValue self, ExprResList&& args);
    ExprRes eval_system_print_unsigned_hex(ulam::LValue self, ExprResList&& args);
    ExprRes eval_system_assert(ulam::LValue self, ExprResList&& args);

    // EventWindow
    ExprRes eval_event_window_aref(ulam::LValue self, ExprResList&& args);

    // Bar
    ExprRes eval_bar_aref(ulam::LValue self, ExprResList&& args);

    ulam::LValue obj_prop(ulam::LValue obj, const std::string_view prop_name);
    ulam::LValue array_item(ulam::LValue array, ulam::RValue&& idx_rval);

    std::ostream& out();

    ExprRes void_res();

    Map _map;
};

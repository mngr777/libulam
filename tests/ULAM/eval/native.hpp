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
    using LValue = ulam::LValue;
    using NodeRef = ulam::Ref<ulam::ast::Node>;

    explicit EvalNative(EvalEnv& env);

    ExprRes call(
        ulam::Ref<ulam::ast::Node> node,
        const std::string_view class_name,
        const std::string_view fun_name,
        LValue self,
        ExprResList&& args);

private:
    using NamePair = std::pair<const std::string_view, const std::string_view>;
    using FunImpl = std::function<ExprRes(NodeRef, LValue, ExprResList&&)>;
    using Map = std::map<NamePair, FunImpl>;

    // System
    ExprRes
    eval_system_print_int(NodeRef node, LValue self, ExprResList&& args);
    ExprRes
    eval_system_print_unsigned(NodeRef node, LValue self, ExprResList&& args);
    ExprRes eval_system_print_unsigned_hex(
        NodeRef node, LValue self, ExprResList&& args);
    ExprRes eval_system_assert(NodeRef node, LValue self, ExprResList&& args);

    // EventWindow
    ExprRes
    eval_event_window_aref(NodeRef node, LValue self, ExprResList&& args);

    // Math
    ExprRes eval_math_max(NodeRef node, LValue self, ExprResList&& args);

    // Bar
    ExprRes eval_bar_aref(NodeRef node, LValue self, ExprResList&& args);

    // utils

    LValue obj_prop(LValue obj, const std::string_view prop_name);
    LValue array_item(LValue array, ulam::RValue&& idx_rval);

    ulam::array_idx_t array_idx(const ulam::RValue& idx_rval);

    std::ostream& out();

    ExprRes void_res();

    Map _map;
};

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
    using FunRef = ulam::Ref<ulam::Fun>;
    using NodeRef = ulam::Ref<ulam::ast::Node>;

    explicit EvalNative(EvalEnv& env);

    ExprRes
    call(NodeRef node, FunRef fun, ulam::LValue self, ExprResList&& args);

private:
    using NamePair = std::pair<const std::string_view, const std::string_view>;
    using FunImpl =
        std::function<ExprRes(NodeRef, FunRef, ulam::LValue, ExprResList&&)>;
    using Map = std::map<NamePair, FunImpl>;

#define _DECLARE_METHOD(name)                                                  \
    ExprRes name(NodeRef, FunRef, ulam::LValue, ExprResList&&)

    // System
    _DECLARE_METHOD(eval_system_print_int);
    _DECLARE_METHOD(eval_system_print_unsigned);
    _DECLARE_METHOD(eval_system_print_unsigned_hex);
    _DECLARE_METHOD(eval_system_assert);

    // EventWindow
    _DECLARE_METHOD(eval_event_window_aref);

    // Math
    _DECLARE_METHOD(eval_math_max);

    // Bar
    _DECLARE_METHOD(eval_bar_aref);

#undef _DECLARE_METHOD

    // utils

    ulam::LValue obj_prop(ulam::LValue obj, const std::string_view prop_name);
    ulam::LValue array_item(ulam::LValue array, ulam::RValue&& idx_rval);

    ulam::array_idx_t array_idx(const ulam::RValue& idx_rval);

    std::ostream& out();

    ExprRes void_res();

    // NOTE: single shared Empty atom, there's no good way to distinguish Bar
    // quark instances
    ulam::RValue _bar_atom;
    Map _map;
};

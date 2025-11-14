#include "./native.hpp"
#include "libulam/semantic/type/class/prop.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/expr_error.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/value/types.hpp>

namespace {

using ExprRes = EvalNative::ExprRes;
using ExprResList = EvalNative::ExprResList;
using EvalExceptAssert = ulam::sema::EvalExceptAssert;

} // namespace

EvalNative::EvalNative(EvalEnv& env): ::EvalHelper{env}, Base{env} {
#define ADD_METHOD(class_name, fun_name, method)                               \
    do {                                                                       \
        namespace ph = std::placeholders;                                      \
        _map.emplace(                                                          \
            NamePair{                                                          \
                std::string_view{class_name}, std::string_view{fun_name}},     \
            std::bind(                                                         \
                std::mem_fn(&EvalNative::method), this, ph::_1, ph::_2));      \
    } while (false)

    ADD_METHOD("System", "print@13i", eval_system_print_int);
    ADD_METHOD("System", "print@14i", eval_system_print_int);
    ADD_METHOD("System", "print@232i", eval_system_print_int);
    ADD_METHOD("System", "print@232u", eval_system_print_unsigned);
    ADD_METHOD("System", "print@13b", eval_system_print_unsigned_hex);
    ADD_METHOD("System", "print@13y", eval_system_print_unsigned_hex);
    ADD_METHOD("System", "assert@11b", eval_system_assert);
    ADD_METHOD("EventWindow", "aref@232i", eval_event_window_aref);
    ADD_METHOD("Bar", "aref@232i", eval_bar_aref);

#undef ADD_METHOD
}

ExprRes EvalNative::call(
    ulam::Ref<ulam::ast::Node> node,
    const std::string_view class_name,
    const std::string_view fun_name,
    ulam::LValue self,
    ExprResList&& args) {
    auto it = _map.find({class_name, fun_name});
    if (it == _map.end()) {
        auto message = std::string{"cannot eval native function "} +
                       std::string{class_name} + "." + std::string{fun_name};
        diag().notice(node->loc_id(), fun_name.size(), message);
        std::exit(0);
        return {ulam::sema::ExprError::CannotEvalNative};
    }
    return (it->second)(self, std::move(args));
}

// System

ExprRes
EvalNative::eval_system_print_int(ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    auto arg = args.pop_front();
    auto rval = arg.move_value().move_rvalue();
    assert(rval.is<ulam::Integer>());
    out() << rval.get<ulam::Integer>() << "\n";
    return void_res();
}

ExprRes
EvalNative::eval_system_print_unsigned(ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    auto arg = args.pop_front();
    auto rval = arg.move_value().move_rvalue();
    assert(rval.is<ulam::Unsigned>());
    out() << rval.get<ulam::Unsigned>() << "\n";
    return void_res();
}

ExprRes EvalNative::eval_system_print_unsigned_hex(
    ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    auto arg = args.pop_front();
    auto rval = arg.move_value().move_rvalue();
    assert(rval.is<ulam::Unsigned>());
    out() << std::hex << rval.get<ulam::Unsigned>() << "\n";
    return void_res();
}

ExprRes EvalNative::eval_system_assert(ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    if (!env().is_true(args.pop_front()))
        throw ulam::sema::EvalExceptAssert("assert failed");
    return void_res();
}

// EventWindow

ExprRes
EvalNative::eval_event_window_aref(ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    auto idx_arg = args.pop_front();

    auto& ctx = test_ctx();
    const auto idx = array_idx(idx_arg.move_value().move_rvalue());
    auto lval = (idx == 0) ? ctx.active_atom() : ctx.neighbor(idx);
    return {builtins().atom_type(), ulam::Value{lval}};
}

// Bar

ExprRes EvalNative::eval_bar_aref(ulam::LValue self, ExprResList&& args) {
    assert(args.size() == 1);
    auto idx_arg = args.pop_front();
    auto lval = obj_prop(self, "val_b");
    lval = array_item(lval, idx_arg.move_value().move_rvalue());
    return {builtins().bool_type(), ulam::Value{lval}};
}

ulam::LValue
EvalNative::obj_prop(ulam::LValue obj, const std::string_view prop_name) {
    auto cls = obj.dyn_cls();
    assert(cls);
    auto sym = cls->get(prop_name);
    assert(sym && sym->is<ulam::Prop>());
    return obj.prop(sym->get<ulam::Prop>());
}

ulam::LValue
EvalNative::array_item(ulam::LValue array, ulam::RValue&& idx_rval) {
    return array.array_access(array_idx(idx_rval), idx_rval.is_consteval());
}

ulam::array_idx_t EvalNative::array_idx(const ulam::RValue& idx_rval) {
    assert(idx_rval.is<ulam::Integer>());
    auto idx_int = idx_rval.get<ulam::Integer>();
    assert(idx_int >= 0);
    return static_cast<ulam::array_idx_t>(idx_int);
}

std::ostream& EvalNative::out() { return std::cout << "System.print: "; }

ExprRes EvalNative::void_res() {
    return {builtins().void_type(), ulam::Value{ulam::RValue{}}};
}

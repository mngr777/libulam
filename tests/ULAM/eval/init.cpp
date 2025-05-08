#include "./init.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include <string>

namespace {
using ExprRes = EvalInit::ExprRes;
}

ExprRes EvalInit::eval_init(
    ulam::Ref<ulam::VarBase> var, ulam::Ref<ulam::ast::InitValue> init) {
    auto type = var->type();
    // for constants, omit consteval cast for scalars
    auto flags_ = flags();
    if ((var->is_const()) && !type->is_array() && !type->is_class())
        flags_ |= evl::NoConstevalCast;
    auto consteval_cast = flags_raii(flags_);
    return Base::eval_init(var, std::move(init));
}

ExprRes EvalInit::eval_class_list(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto res = Base::eval_class_list(var, cls, list, depth);
    if (!has_flag(evl::NoCodegen) && depth == 1) {
        // var_name ( args ) Self .
        auto data = exp::data_combine(str(var->name_id()), exp::data(res), ".");
        exp::set_data(res, data);
    }
    return res;
}

ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto flags_ = flags();
    if (depth > 1)
        flags_ |= evl::NoConstFold;
    auto no_fold_raii = flags_raii(flags_);
    auto array = Base::eval_array_list(var, array_type, list, depth);
    if (!has_flag(evl::NoCodegen)) {
        auto array_data = array.has_data() ? exp::data(array) : std::string{};
        exp::set_data(array, "{ " + array_data + " }");
    }
    return array;
}

ExprRes EvalInit::eval_class_map(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::InitMap> map,
    unsigned depth) {
    auto flags_ = flags();
    if (depth > 1)
        flags_ |= evl::NoConstFold;
    auto no_fold_raii = flags_raii(flags_);
    auto obj = Base::eval_class_map(var, cls, map, depth);
    if (!has_flag(evl::NoCodegen)) {
        auto obj_data = obj.has_data() ? exp::data(obj) : std::string{};
        exp::set_data(obj, "{ " + obj_data + " }");
    }
    return obj;
}

ExprRes EvalInit::array_set(
    ulam::Ref<ulam::VarBase> var,
    ExprRes&& array,
    ulam::array_idx_t idx,
    ExprRes&& item,
    bool autofill,
    unsigned depth) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        if (!autofill)
            exp::append(array, value_str(var, item, depth), ", ");
        data = exp::data(array);
    }
    array = Base::array_set(
        var, std::move(array), idx, std::move(item), autofill, depth);
    if (!data.empty())
        exp::set_data(array, data);
    return std::move(array);
}

ExprRes EvalInit::obj_set(
    ulam::Ref<ulam::VarBase> var,
    ExprRes&& obj,
    ulam::Ref<ulam::Prop> prop,
    ExprRes&& prop_res,
    unsigned depth) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        auto label = "." + std::string{str(prop->name_id())};
        auto value = value_str(var, prop_res, depth);
        exp::append(obj, exp::data_combine(label, "=", value), ", ");
        data = exp::data(obj);
    }
    obj = Base::obj_set(var, std::move(obj), prop, std::move(prop_res), depth);
    if (!data.empty())
        exp::set_data(obj, data);
    return std::move(obj);
}

std::string EvalInit::value_str(
    ulam::Ref<ulam::VarBase> var, const ExprRes& res, unsigned depth) {
    auto data = exp::data(res);
    auto type = res.type();
    bool no_fold = depth > 1 || type->is_array() || type->is_class();
    if (!no_fold && res.value().is_consteval() && !(var->is_const())) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            Stringifier stringifier{program()};
            stringifier.options.unary_as_unsigned_lit = true;
            data = stringifier.stringify(res.type(), rval);
        });
    }
    return data;
}

#include "./init.hpp"
#include "../out.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include "./util.hpp"
#include <libulam/semantic/var.hpp>
#include <string>

namespace {
using ExprRes = EvalInit::ExprRes;

bool is_local_var(ulam::Ref<ulam::Var> var) {
    return var->is_local() && !var->has_flag(ulam::Var::ClassParam);
}

} // namespace

ExprRes EvalInit::eval_init(
    ulam::Ref<ulam::VarBase> var, ulam::Ref<ulam::ast::InitValue> init) {
    auto type = var->type();
    // for constants, omit consteval cast for scalars
    EvalEnv::FlagsRaii fr{};
    if ((var->is_const()) && !type->is_array() && !type->is_class())
        fr = env().add_flags_raii(evl::NoConstevalCast);
    return Base::eval_init(var, std::move(init));
}

void EvalInit::var_init_expr(
    ulam::Ref<ulam::Var> var, ExprRes&& init, bool in_expr) {
    std::string data;
    if (!in_expr && codegen_enabled())
        data = exp::data(init);
    Base::var_init_expr(var, std::move(init), in_expr);
    if (!in_expr && is_local_var(var) && codegen_enabled()) {
        gen().append("=");
        gen().append(data + "; ");
    }
}

void EvalInit::var_init_default(ulam::Ref<ulam::Var> var, bool in_expr) {
    Base::var_init_default(var, in_expr);
    if (!in_expr && is_local_var(var) && codegen_enabled())
        gen().append("; ", true);
}

void EvalInit::var_init_common(ulam::Ref<ulam::Var> var, bool in_expr) {
    Base::var_init_common(var, in_expr);
    if (!in_expr && is_local_var(var) && codegen_enabled()) {
        auto strf = gen().make_strf();
        gen().append(out::var_def_str(str_pool(), strf, var));
    }
}

ExprRes EvalInit::eval_class_list(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto res = Base::eval_class_list(var, cls, list, depth);
    if (!has_flag(evl::NoCodegen) && depth == 1) {
        if (list->child_num() > 0) {
            // var_name ( args ) Self .
            auto data =
                exp::data_combine(str(var->name_id()), exp::data(res), ".");
            exp::set_data(res, data);
        }
    }
    return res;
}

ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::LValue default_lval,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    EvalEnv::FlagsRaii fr{};
    if (depth > 1)
        fr = env().add_flags_raii(evl::NoConstFold);
    auto array =
        Base::eval_array_list(var, array_type, default_lval, list, depth);
    if (!has_flag(evl::NoCodegen)) {
        auto data = array.has_data() ? "{ " + exp::data(array) + " }"
                                     : std::string{"{ }"};
        exp::set_data(array, data);
    }
    return array;
}

ExprRes EvalInit::eval_class_map(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Class> cls,
    ulam::LValue default_lval,
    ulam::Ref<ulam::ast::InitMap> map,
    unsigned depth) {
    EvalEnv::FlagsRaii fr{};
    if (depth > 1)
        fr = env().add_flags_raii(evl::NoConstFold);
    auto obj = Base::eval_class_map(var, cls, default_lval, map, depth);
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

ExprRes EvalInit::construct_obj(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::InitList> arg_list,
    ExprResList&& args) {
    bool is_default = args.empty();
    auto res = Base::construct_obj(var, cls, arg_list, std::move(args));
    if (!has_flag(evl::NoCodegen)) {
        if (is_default)
            exp::set_data(res, "{ }");
    }
    return res;
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
    // NOTE: not folding for const var values
    if (!no_fold && util::can_fold(res) && !(var->is_const())) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            Stringifier stringifier{program()};
            stringifier.options.unary_as_unsigned_lit = true;
            data = stringifier.stringify(res.type(), rval);
        });
    }
    return data;
}

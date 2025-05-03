#include "./init.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_init(
    ulam::Ref<ulam::VarBase> var, ulam::Ref<ulam::ast::InitValue> init) {
    auto type = var->type();
    // for constants, omit consteval cast for scalars
    auto flags_ = flags();
    if ((var->is_const()) && !type->is_array() && !type->is_class())
        flags_ |= evl::NoConstevalCast;
    auto consteval_cast = flags_raii(flags_);
    return Base::eval_init(var, std::move(init));
}

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto array = Base::eval_array_list(var, array_type, list, depth);
    if (has_flag(evl::NoCodegen))
        return array;

    auto array_data = array.has_data() ? exp::data(array) : std::string{};
    auto data = exp::data_combine("{ " + array_data + " }");
    exp::set_data(array, data);
    return array;
}

ulam::sema::ExprRes EvalInit::eval_array_list_item(
    ulam::Ref<ulam::VarBase> var,
    ulam::Ref<ulam::Type> type,
    Variant& item_v,
    unsigned depth) {
    auto res = Base::eval_array_list_item(var, type, item_v, depth);
    if (has_flag(evl::NoCodegen))
        return res;

    // NOTE: _not_ replacing _const_ var consteval variables, see t3250, t3882
    if (res.value().is_consteval() && !(var->is_const())) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            Stringifier stringifier{program()};
            exp::set_data(res, stringifier.stringify(res.type(), rval));
        });
    }
    return res;
}

ulam::sema::ExprRes EvalInit::array_set(
    ulam::Ref<ulam::VarBase> var,
    ulam::sema::ExprRes&& array,
    ulam::array_idx_t idx,
    ulam::sema::ExprRes&& item,
    bool autofill) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        if (!autofill)
            exp::append(array, exp::data(item), ", ");
        data = exp::data(array);
    }
    array =
        Base::array_set(var, std::move(array), idx, std::move(item), autofill);
    if (!data.empty())
        exp::set_data(array, data);
    return std::move(array);
}

// ulam::sema::ExprRes EvalInit::make_obj(ulam::Ref<ulam::Class> cls) {
//     return {}; // TODO
// }

// ulam::sema::ExprRes EvalInit::obj_set(
//     ulam::sema::ExprRes&& obj,
//     ulam::Ref<ulam::Prop> prop,
//     ulam::sema::ExprRes&& prop_res) {
//     return {}; // TODO
// }

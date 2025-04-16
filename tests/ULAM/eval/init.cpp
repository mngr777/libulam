#include "./init.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto array = ulam::sema::EvalInit::eval_array_list(array_type, list, depth);
    auto data = exp::data_combine("{", exp::data(array), "}");
    exp::set_data(array, data);
    return array;
}

ulam::sema::ExprRes EvalInit::eval_array_list_item(
    ulam::Ref<ulam::Type> type, Variant& item_v, unsigned depth) {
    auto no_consteval_cast = flags_raii(flags() | evl::NoConstevalCast);
    return ulam::sema::EvalInit::eval_array_list_item(type, item_v, depth);
}

ulam::sema::ExprRes EvalInit::array_set(
    ulam::sema::ExprRes&& array,
    ulam::array_idx_t idx,
    ulam::sema::ExprRes&& item,
    bool autofill) {

    if (!autofill)
        exp::append(array, exp::data(item), ",");
    std::string data = exp::data(array);

    array = ulam::sema::EvalInit::array_set(
        std::move(array), idx, std::move(item), autofill);

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

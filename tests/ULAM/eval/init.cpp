#include "./init.hpp"
#include "./flags.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto array = ulam::sema::EvalInit::eval_array_list(array_type, list, depth);
    if (array.has_data()) {
        auto data = array.data<std::string>();
        array.set_data("{ " + data + " }");
    } else {
        array.uns_data();
    }
    return array;
}

ulam::sema::ExprRes EvalInit::eval_array_list_item(
    ulam::Ref<ulam::Type> type, Variant& item_v, unsigned depth) {
    auto no_consteval_cast = flags_raii(flags() | evl::NoConstevalCast);
    return ulam::sema::EvalInit::eval_array_list_item(type, item_v, depth);
}

ulam::sema::ExprRes
EvalInit::make_array(ulam::Ref<ulam::ArrayType> array_type) {
    auto array = ulam::sema::EvalInit::make_array(array_type);
    array.set_data<std::string>("");
    return array;
}

ulam::sema::ExprRes EvalInit::array_set(
    ulam::sema::ExprRes&& array,
    ulam::array_idx_t idx,
    ulam::sema::ExprRes&& item,
    bool autofill) {
    std::string array_data;
    if (array.has_data() && item.has_data()) {
        array_data = array.data<std::string>();
        if (!autofill) {
            if (!array_data.empty())
                array_data += ",";
            array_data += item.data<std::string>();
        }
    }
    array = ulam::sema::EvalInit::array_set(
        std::move(array), idx, std::move(item), autofill);
    if (!array_data.empty()) {
        array.set_data(array_data);
    } else {
        array.uns_data();
    }
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

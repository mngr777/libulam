#include "./init.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto array = ulam::sema::EvalInit::eval_array_list(array_type, list, depth);
    auto data = array.data<std::string>("");
    if (!data.empty()) {
        array.set_data("(" + data + ")");
    } else {
        array.uns_data();
    }
    return array;
}

ulam::sema::ExprRes
EvalInit::make_array(ulam::Ref<ulam::ArrayType> array_type) {
    auto array = ulam::sema::EvalInit::make_array(array_type);
    array.set_data("");
    return array;
}

ulam::sema::ExprRes EvalInit::array_set(
    ulam::sema::ExprRes&& array,
    ulam::array_idx_t idx,
    ulam::sema::ExprRes&& item) {
    std::string array_data;
    if (array.has_data() && item.has_data()) {
        array_data = array.data<std::string>();
        if (!array_data.empty())
            array_data += ",";
        array_data += item.data<std::string>();
    }
    array = ulam::sema::EvalInit::array_set(std::move(array), idx, std::move(item));
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

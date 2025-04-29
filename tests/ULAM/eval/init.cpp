#include "./init.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_init(
    ulam::Ref<ulam::Type> type,
    ulam::Ref<ulam::ast::InitValue> init,
    bool is_const) {
    // for constants, omit consteval cast for scalars
    auto flags_ = flags();
    if (is_const)
        flags_ |= evl::NoConstevalCast;
    auto no_consteval_cast = flags_raii(flags_);
    return Base::eval_init(type, std::move(init), is_const);
}

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth) {
    auto array = Base::eval_array_list(array_type, list, depth);
    auto array_data = array.has_data() ? exp::data(array) : std::string{};
    auto data = exp::data_combine("{ " + array_data + " }");
    exp::set_data(array, data);
    return array;
}

ulam::sema::ExprRes EvalInit::eval_array_list_item(
    ulam::Ref<ulam::Type> type, Variant& item_v, unsigned depth) {
    auto no_consteval_cast = flags_raii(flags() | evl::NoConstevalCast);
    auto res = Base::eval_array_list_item(type, item_v, depth);
    if (res.value().is_consteval()) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            Stringifier stringifier{program()};
            exp::set_data(res, stringifier.stringify(res.type(), rval));
        });
    }
    return res;
}

ulam::sema::ExprRes EvalInit::array_set(
    ulam::sema::ExprRes&& array,
    ulam::array_idx_t idx,
    ulam::sema::ExprRes&& item,
    bool autofill) {

    if (!autofill)
        exp::append(array, exp::data(item), ", ");
    std::string data = exp::data(array);
    array = Base::array_set(std::move(array), idx, std::move(item), autofill);
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

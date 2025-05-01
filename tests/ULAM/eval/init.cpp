#include "./init.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include <string>

ulam::sema::ExprRes EvalInit::eval_init(
    ulam::Ref<ulam::Type> type,
    ulam::Ref<ulam::ast::InitValue> init,
    ulam::Var::flags_t var_flags) {
    // for constants, omit consteval cast for scalars
    auto flags_ = flags();
    if ((var_flags & ulam::Var::Const) && !type->is_array() &&
        !type->is_class())
        flags_ |= evl::NoConstevalCast;
    auto consteval_cast = flags_raii(flags_);
    return Base::eval_init(type, std::move(init), var_flags);
}

ulam::sema::ExprRes EvalInit::eval_array_list(
    ulam::Ref<ulam::ArrayType> array_type,
    ulam::Ref<ulam::ast::InitList> list,
    unsigned depth,
    ulam::Var::flags_t var_flags) {
    auto array = Base::eval_array_list(array_type, list, depth, var_flags);
    if (has_flag(evl::NoCodegen))
        return array;

    auto array_data = array.has_data() ? exp::data(array) : std::string{};
    auto data = exp::data_combine("{ " + array_data + " }");
    exp::set_data(array, data);
    return array;
}

ulam::sema::ExprRes EvalInit::eval_array_list_item(
    ulam::Ref<ulam::Type> type,
    Variant& item_v,
    unsigned depth,
    ulam::Var::flags_t var_flags) {
    auto res = Base::eval_array_list_item(type, item_v, depth, var_flags);
    if (has_flag(evl::NoCodegen))
        return res;

    // NOTE: _not_ replacing _const_ var consteval variables, see t3250, t3882
    if (res.value().is_consteval() && !(var_flags & ulam::Var::Const)) {
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
    bool autofill,
    ulam::Var::flags_t var_flags) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        if (!autofill)
            exp::append(array, exp::data(item), ", ");
        data = exp::data(array);
    }
    array = Base::array_set(
        std::move(array), idx, std::move(item), autofill, var_flags);
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

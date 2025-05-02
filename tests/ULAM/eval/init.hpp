#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/var.hpp>

class EvalInit : public ulam::sema::EvalInit {
public:
    using Base = ulam::sema::EvalInit;
    using Base::EvalInit;

    virtual ulam::sema::ExprRes eval_init(
        ulam::Ref<ulam::Type> type,
        ulam::Ref<ulam::ast::InitValue> init,
        ulam::Var::flags_t var_flags) override;

protected:
    ulam::sema::ExprRes cast_expr_res(
        ulam::Ref<ulam::Type> type,
        ulam::Ref<ulam::ast::Expr> expr,
        ulam::sema::ExprRes&& res,
        unsigned depth,
        ulam::Var::flags_t var_flags) override;

    ulam::sema::ExprRes eval_array_list(
        ulam::Ref<ulam::ArrayType> array_type,
        ulam::Ref<ulam::ast::InitList> list,
        unsigned depth,
        ulam::Var::flags_t var_flags) override;

    ulam::sema::ExprRes eval_array_list_item(
        ulam::Ref<ulam::Type> type,
        Variant& item_v,
        unsigned depth,
        ulam::Var::flags_t var_flags) override;

    virtual ulam::sema::ExprRes array_set(
        ulam::sema::ExprRes&& array,
        ulam::array_idx_t idx,
        ulam::sema::ExprRes&& item,
        bool autofill,
        ulam::Var::flags_t var_flags) override;

    // virtual ulam::sema::ExprRes make_obj(ulam::Ref<ulam::Class> cls);

    // virtual ulam::sema::ExprRes obj_set(
    //     ulam::sema::ExprRes&& obj,
    //     ulam::Ref<ulam::Prop> prop,
    //     ulam::sema::ExprRes&& prop_res);
};

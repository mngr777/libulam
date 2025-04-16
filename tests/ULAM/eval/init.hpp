#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>

class EvalInit : public ulam::sema::EvalInit {
public:
    using ulam::sema::EvalInit::EvalInit;

protected:
    ulam::sema::ExprRes eval_array_list(
        ulam::Ref<ulam::ArrayType> array_type,
        ulam::Ref<ulam::ast::InitList> list,
        unsigned depth) override;

    ulam::sema::ExprRes eval_array_list_item(
        ulam::Ref<ulam::Type> type, Variant& item_v, unsigned depth) override;

    virtual ulam::sema::ExprRes array_set(
        ulam::sema::ExprRes&& array,
        ulam::array_idx_t idx,
        ulam::sema::ExprRes&& item,
        bool autofill) override;

    // virtual ulam::sema::ExprRes make_obj(ulam::Ref<ulam::Class> cls);

    // virtual ulam::sema::ExprRes obj_set(
    //     ulam::sema::ExprRes&& obj,
    //     ulam::Ref<ulam::Prop> prop,
    //     ulam::sema::ExprRes&& prop_res);
};

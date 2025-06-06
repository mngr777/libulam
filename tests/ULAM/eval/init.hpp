#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/var/base.hpp>

class EvalInit : public ulam::sema::EvalInit {
public:
    using Base = ulam::sema::EvalInit;
    using ExprRes = ulam::sema::ExprRes;
    using ExprResList = ulam::sema::ExprResList;

    using Base::EvalInit;

    ulam::sema::ExprRes eval_init(
        ulam::Ref<ulam::VarBase> var,
        ulam::Ref<ulam::ast::InitValue> init) override;

protected:
    ExprRes eval_class_list(
        ulam::Ref<ulam::VarBase> var,
        ulam::Ref<ulam::Class> cls,
        ulam::Ref<ulam::ast::InitList> list,
        unsigned depth) override;

    ExprRes eval_array_list(
        ulam::Ref<ulam::VarBase> var,
        ulam::Ref<ulam::ArrayType> array_type,
        ulam::Ref<ulam::ast::InitList> list,
        unsigned depth) override;

    ExprRes eval_class_map(
        ulam::Ref<ulam::VarBase> var,
        ulam::Ref<ulam::Class> cls,
        ulam::Ref<ulam::ast::InitMap> map,
        unsigned depth) override;

    ExprRes array_set(
        ulam::Ref<ulam::VarBase> var,
        ExprRes&& array,
        ulam::array_idx_t idx,
        ulam::sema::ExprRes&& item,
        bool autofill,
        unsigned depth) override;

    ExprRes construct_obj(
        ulam::Ref<ulam::VarBase> var,
        ulam::Ref<ulam::Class> cls,
        ulam::Ref<ulam::ast::InitList> arg_list,
        ExprResList&& args) override;

    ExprRes obj_set(
        ulam::Ref<ulam::VarBase> var,
        ExprRes&& obj,
        ulam::Ref<ulam::Prop> prop,
        ExprRes&& prop_res,
        unsigned depth) override;

private:
    std::string
    value_str(ulam::Ref<ulam::VarBase> var, const ExprRes& res, unsigned depth);
};

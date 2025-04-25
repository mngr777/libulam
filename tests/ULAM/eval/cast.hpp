#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/typed_value.hpp>

class EvalCast : public ulam::sema::EvalCast {
public:
    using Base = ulam::sema::EvalCast;
    using Base::EvalCast;

    ulam::sema::ExprRes cast(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Type> type,
        ulam::sema::ExprRes&& arg,
        bool expl = false) override;

    ulam::sema::ExprRes cast(
        ulam::Ref<ulam::ast::Node> node,
        ulam::BuiltinTypeId bi_type_id,
        ulam::sema::ExprRes&& arg,
        bool expl = false) override;

    ulam::sema::ExprRes cast_to_idx(
        ulam::Ref<ulam::ast::Node> node, ulam::sema::ExprRes&& arg) override;

protected:
    ulam::sema::ExprRes cast_class_default(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Type> to,
        ulam::sema::ExprRes&& arg,
        bool expl) override;

    virtual ulam::sema::ExprRes cast_class_fun(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::sema::ExprRes&& arg,
        bool expl) override;

    virtual ulam::sema::ExprRes cast_class_fun_after(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Type> to,
        ulam::sema::ExprRes&& arg,
        bool expl) override;

    void update_res(
        ulam::sema::ExprRes& res, EvalCast::CastStatus status, bool expl);
};

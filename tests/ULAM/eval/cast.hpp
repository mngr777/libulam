#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/typed_value.hpp>

class EvalCast : public ulam::sema::EvalCast {
public:
    using Base = ulam::sema::EvalCast;
    using ExprRes = ulam::sema::ExprRes;

    using Base::EvalCast;

    ExprRes cast(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Type> type,
        ExprRes&& arg,
        bool expl = false) override;

    ExprRes cast(
        ulam::Ref<ulam::ast::Node> node,
        ulam::BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl = false) override;

    ExprRes
    cast_to_idx(ulam::Ref<ulam::ast::Node> node, ExprRes&& arg) override;

protected:
    ExprRes cast_atom_to_nonelement_empty(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Class> to,
        ExprRes&& arg) override;

    ExprRes cast_class_fun(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ExprRes&& arg,
        bool expl) override;

    ExprRes cast_default(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Type> to,
        ExprRes&& arg,
        bool expl) override;

    ExprRes cast_default(
        ulam::Ref<ulam::ast::Node> node,
        ulam::BuiltinTypeId bi_type_id,
        ExprRes&& arg,
        bool expl) override;

    void update_res(ExprRes& res, EvalCast::CastStatus status, bool expl);

    ExprRes take_ref(ulam::Ref<ulam::ast::Node> node, ExprRes&& arg) override;
    ExprRes deref(ExprRes&& arg) override;

    void update_res(ExprRes& res, bool expl);
};

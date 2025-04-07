#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/typed_value.hpp>

class EvalCast : public ulam::sema::EvalCast {
public:
    using ulam::sema::EvalCast::EvalCast;

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
};

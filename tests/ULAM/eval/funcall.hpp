#include "./env.hpp"
#include "./helper.hpp"
#include <libulam/sema/eval/funcall.hpp>

class EvalFuncall : public EvalHelper, public ulam::sema::EvalFuncall {
public:
    using Base = ulam::sema::EvalFuncall;
    using ExprRes = ulam::sema::ExprRes;
    using ExprResList = ulam::sema::ExprResList;

    EvalFuncall(EvalEnv& env): ::EvalHelper{env}, Base{env} {}

protected:
    ExprRes construct_funcall(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Class> cls,
        ulam::Ref<ulam::Fun> fun,
        ulam::RValue&& rval,
        ExprResList&& args) override;

    ExprRes funcall_callable(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ExprRes&& callable,
        ExprResList&& args) override;

    ExprRes funcall_obj(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ExprRes&& obj,
        ExprResList&& args) override;

    ExprRes do_funcall(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::LValue self,
        ExprResList&& args) override;

    ExprRes do_funcall_native(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::LValue self,
        ExprResList&& args) override;

    ExprResList cast_args(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ExprResList&& args) override;

    ExprRes cast_arg(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::Ref<ulam::Var> param,
        ulam::Ref<ulam::Type> to,
        ExprRes&& arg) override;

    std::string arg_data(const ExprResList& args);
};

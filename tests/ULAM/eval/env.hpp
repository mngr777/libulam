#pragma once
#include "./codegen.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>

class EvalEnv : public ulam::sema::EvalEnv {
public:
    using Base = ulam::sema::EvalEnv;
    using CondRes = ulam::sema::CondRes;
    using ExprRes = ulam::sema::ExprRes;
    using ExprResList = ulam::sema::ExprResList;

    explicit EvalEnv(
        ulam::Ref<ulam::Program> program,
        ulam::sema::eval_flags_t flags = ulam::sema::evl::NoFlags):
        Base{program, flags}, _codegen{program} {}

    ExprRes eval(ulam::Ref<ulam::ast::Block> block) override;

    void eval_stmt(ulam::Ref<ulam::ast::Stmt> stmt) override;

    void eval_which(ulam::Ref<ulam::ast::Which> which) override;

    ExprRes eval_expr(ulam::Ref<ulam::ast::Expr> expr) override;

    ExprRes eval_equal(
        ulam::Ref<ulam::ast::Expr> node,
        ulam::Ref<ulam::ast::Expr> l_node,
        ExprRes&& left,
        ulam::Ref<ulam::ast::Expr> r_node,
        ExprRes&& right) override;

    CondRes eval_cond(ulam::Ref<ulam::ast::Cond> cond) override;

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

    ExprRes cast_to_idx(
        ulam::Ref<ulam::ast::Node> node,
        ExprRes&& arg) override;

    bool init_var(
        ulam::Ref<ulam::Var> var,
        ulam::Ref<ulam::ast::InitValue> init,
        bool in_expr) override;

    bool init_prop(
        ulam::Ref<ulam::Prop> prop,
        ulam::Ref<ulam::ast::InitValue> init) override;

    ExprRes construct(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Class> cls,
        ExprResList&& args) override;

    ExprRes call(
        ulam::Ref<ulam::ast::Node> node,
        ExprRes&& callable,
        ExprResList&& args) override;

    ExprRes funcall(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ExprRes&& obj,
        ExprResList&& args) override;

    Codegen& gen() { return _codegen; }

private:
    Codegen _codegen;
};

#pragma once
#include "./env.hpp"
#include "./helper.hpp"
#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/stmts.hpp>

class EvalCond : public EvalHelper, public ulam::sema::EvalCond {
public:
    using Base = ulam::sema::EvalCond;
    using CondRes = ulam::sema::CondRes;

    EvalCond(EvalEnv& env): ::EvalHelper{env}, Base{env} {}

    CondRes eval_as_cond(ulam::Ref<ulam::ast::AsCond> as_cond);

protected:
    CondRes eval_expr(ulam::Ref<ulam::ast::Expr> expr);
};

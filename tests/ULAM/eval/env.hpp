#pragma once
#include "./codegen.hpp"
#include "./test_context.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value.hpp>
#include <limits>

class EvalEnv : public ulam::sema::EvalEnv {
public:
    using Base = ulam::sema::EvalEnv;
    using CondRes = ulam::sema::CondRes;
    using ExprRes = ulam::sema::ExprRes;
    using ExprResList = ulam::sema::ExprResList;

    class EvalTestContextRaii {
        friend EvalEnv;

    public:
        EvalTestContextRaii();
        ~EvalTestContextRaii();

        EvalTestContextRaii(EvalTestContextRaii&& other);
        EvalTestContextRaii& operator=(EvalTestContextRaii&& other);

    private:
        EvalTestContextRaii(EvalEnv& env, ulam::LValue active_atom);

        EvalEnv* _env;
    };

    explicit EvalEnv(
        ulam::Ref<ulam::Program> program,
        ulam::sema::eval_flags_t flags = ulam::sema::evl::NoFlags):
        Base{program, flags}, _codegen{program} {}

    ExprRes eval(ulam::Ref<ulam::ast::Block> block) override;

    EvalTestContextRaii test_ctx_raii(ulam::LValue active_atom);

    Codegen& gen() { return _codegen; }

    EvalTestContext& test_ctx();

    void set_status(int status) { _status = status; }
    int status() { return _status; }

protected:
    ExprRes eval_with_cast(EvalWithCast eval) override;
    CondRes eval_with_cond(EvalWithCond eval) override;
    ExprRes eval_with_expr_visitor(EvalWithExprVisitor eval) override;
    bool eval_with_init(EvalWithInit eval) override;
    ExprRes eval_with_funcall(EvalWithFuncall eval) override;
    void eval_with_stmt_visitor(EvalWithStmtVisitor eval) override;
    void eval_with_which(EvalWithWhich eval) override;


private:
    Codegen _codegen;
    EvalTestContext _test_ctx{};
    int _status{std::numeric_limits<int>::min()};
};

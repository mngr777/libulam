#include "./env.hpp"
#include "./cast.hpp"
#include "./cond.hpp"
#include "./expr_visitor.hpp"
#include "./flags.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "./stmt_visitor.hpp"
#include "./which.hpp"
#include <libulam/assert.hpp>

using CondRes = EvalEnv::CondRes;
using ExprRes = EvalEnv::ExprRes;
using ExprResList = EvalEnv::ExprResList;

// EvalTestContextRaii

EvalEnv::EvalTestContextRaii::EvalTestContextRaii(): _env{} {}

EvalEnv::EvalTestContextRaii::~EvalTestContextRaii() {
    if (_env) {
        ulam_assert(!_env->_test_ctx.empty());
        _env->_test_ctx = {};
    }
}

EvalEnv::EvalTestContextRaii::EvalTestContextRaii(EvalTestContextRaii&& other) {
    operator=(std::move(other));
}

EvalEnv::EvalTestContextRaii&
EvalEnv::EvalTestContextRaii::operator=(EvalTestContextRaii&& other) {
    std::swap(_env, other._env);
    return *this;
}

EvalEnv::EvalTestContextRaii::EvalTestContextRaii(
    EvalEnv& env, ulam::LValue active_atom):
    _env{&env} {
    ulam_assert(env._test_ctx.empty());
    env._test_ctx = {env.builtins(), active_atom};
}

// EvalEnv

ExprRes EvalEnv::eval(ulam::Ref<ulam::ast::Block> block) {
    // codegen
    Base::eval(block);

    // exec
    auto fr = add_flags_raii(evl::NoCodegen);
    return Base::eval(block);
}

EvalTestContext& EvalEnv::test_ctx() {
    ulam_assert(!_test_ctx.empty());
    return _test_ctx;
}

EvalEnv::EvalTestContextRaii EvalEnv::test_ctx_raii(ulam::LValue active_atom) {
    return {*this, active_atom};
}

ExprRes EvalEnv::eval_with_cast(EvalWithCast eval) {
    EvalCast ec{*this};
    return eval(ec);
}

CondRes EvalEnv::eval_with_cond(EvalWithCond eval) {
    EvalCond ec{*this};
    return eval(ec);
}

ExprRes EvalEnv::eval_with_expr_visitor(EvalWithExprVisitor eval) {
    EvalExprVisitor ev{*this};
    return eval(ev);
}

bool EvalEnv::eval_with_init(EvalWithInit eval) {
    EvalInit ei{*this};
    return eval(ei);
}

ExprRes EvalEnv::eval_with_funcall(EvalWithFuncall eval) {
    EvalFuncall ef{*this};
    return eval(ef);
}

void EvalEnv::eval_with_stmt_visitor(EvalWithStmtVisitor eval) {
    EvalStmtVisitor es{*this};
    return eval(es);
}

void EvalEnv::eval_with_which(EvalWithWhich eval) {
    EvalWhich ew{*this};
    return eval(ew);
}

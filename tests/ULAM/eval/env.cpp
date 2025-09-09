#include "./env.hpp"
#include "./cast.hpp"
#include "./which.hpp"
#include "./cond.hpp"
#include "./expr_visitor.hpp"
#include "./flags.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "./visitor.hpp"

using CondRes = EvalEnv::CondRes;
using ExprRes = EvalEnv::ExprRes;
using ExprResList = EvalEnv::ExprResList;

ExprRes EvalEnv::eval(ulam::Ref<ulam::ast::Block> block) {
    // codegen
    // NOTE: ulam::sema::evl::NoExec set on first function call, see `funcall`
    Base::eval(block);

    // exec
    auto fr = add_flags_raii(evl::NoCodegen);
    return Base::eval(block);
}

void EvalEnv::eval_stmt(ulam::Ref<ulam::ast::Stmt> stmt) {
    EvalVisitor vis{*this};
    do_eval_stmt(vis, stmt);
}

void EvalEnv::eval_which(ulam::Ref<ulam::ast::Which> which) {
    EvalWhich ew{*this};
    do_eval_which(ew, which);
}

ExprRes EvalEnv::eval_expr(ulam::Ref<ulam::ast::Expr> expr) {
    EvalExprVisitor ev{*this};
    return do_eval_expr(ev, expr);
}

ExprRes EvalEnv::eval_equal(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::ast::Expr> l_node,
    ExprRes&& left,
    ulam::Ref<ulam::ast::Expr> r_node,
    ExprRes&& right) {
    EvalExprVisitor ev{*this};
    return do_eval_equal(
        ev, node, l_node, std::move(left), r_node, std::move(right));
}

CondRes EvalEnv::eval_cond(ulam::Ref<ulam::ast::Cond> cond) {
    EvalCond ec{*this};
    return do_eval_cond(ec, cond);
}

ExprRes EvalEnv::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Type> type,
    ExprRes&& arg,
    bool expl) {
    EvalCast cast{*this};
    return do_cast(cast, node, type, std::move(arg), expl);
}

ExprRes EvalEnv::cast(
    ulam::Ref<ulam::ast::Node> node,
    ulam::BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    EvalCast cast{*this};
    return do_cast(cast, node, bi_type_id, std::move(arg), expl);
}

ExprRes EvalEnv::cast_to_idx(ulam::Ref<ulam::ast::Node> node, ExprRes&& arg) {
    EvalCast cast{*this};
    return do_cast_to_idx(cast, node, std::move(arg));
}

bool EvalEnv::init_var(
    ulam::Ref<ulam::Var> var,
    ulam::Ref<ulam::ast::InitValue> init,
    bool in_expr) {
    EvalInit ei{*this};
    return do_init_var(ei, var, init, in_expr);
}

bool EvalEnv::init_prop(
    ulam::Ref<ulam::Prop> prop, ulam::Ref<ulam::ast::InitValue> init) {
    EvalInit ei{*this};
    return do_init_prop(ei, prop, init);
}

ExprRes EvalEnv::construct(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Class> cls,
    ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_construct(ef, node, cls, std::move(args));
}

ExprRes EvalEnv::call(
    ulam::Ref<ulam::ast::Node> node, ExprRes&& callable, ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_call(ef, node, std::move(callable), std::move(args));
}

ExprRes EvalEnv::funcall(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ExprRes&& obj,
    ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_funcall(ef, node, fun, std::move(obj), std::move(args));
}

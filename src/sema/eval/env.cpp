#include <algorithm>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/eval/which.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <utility>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalEnv] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

// EvalEnv::ScopeSwitchRaii

EvalEnv::ScopeSwitchRaii::ScopeSwitchRaii(): _env{}, _old_scope_override{} {}

EvalEnv::ScopeSwitchRaii::~ScopeSwitchRaii() {
    if (_env)
        _env->_scope_override = _old_scope_override;
}

EvalEnv::ScopeSwitchRaii::ScopeSwitchRaii(ScopeSwitchRaii&& other) {
    operator=(std::move(other));
}

EvalEnv::ScopeSwitchRaii&
EvalEnv::ScopeSwitchRaii::operator=(ScopeSwitchRaii&& other) {
    std::swap(_env, other._env);
    std::swap(_old_scope_override, other._old_scope_override);
    return *this;
}

EvalEnv::ScopeSwitchRaii::ScopeSwitchRaii(EvalEnv& env, Scope* scope):
    _env{&env}, _old_scope_override{env._scope_override} {
    env._scope_override = scope;
}

// EvalEnv::FlagsRaii

EvalEnv::FlagsRaii::FlagsRaii(): _env{}, _old_flags{} {}

EvalEnv::FlagsRaii::~FlagsRaii() {
    if (_env)
        _env->_flags = _old_flags;
}

EvalEnv::FlagsRaii::FlagsRaii(FlagsRaii&& other) {
    operator=(std::move(other));
}

EvalEnv::FlagsRaii& EvalEnv::FlagsRaii::operator=(FlagsRaii&& other) {
    std::swap(_env, other._env);
    std::swap(_old_flags, other._old_flags);
    return *this;
}

EvalEnv::FlagsRaii::FlagsRaii(EvalEnv& env, eval_flags_t flags):
    _env{&env}, _old_flags{env._flags} {
    env._flags = flags;
}

// EvalEnv::VarDefaultsRaii

EvalEnv::VarDefaultsRaii::VarDefaultsRaii(): _env{} {}

EvalEnv::VarDefaultsRaii::~VarDefaultsRaii() {
    if (_env) {
        for (auto var : _var_set)
            _env->_var_defaults.erase(var);
    }
}

EvalEnv::VarDefaultsRaii::VarDefaultsRaii(VarDefaultsRaii&& other) {
    operator=(std::move(other));
}

EvalEnv::VarDefaultsRaii&
EvalEnv::VarDefaultsRaii::operator=(VarDefaultsRaii&& other) {
    std::swap(_env, other._env);
    std::swap(_var_set, other._var_set);
    return *this;
}

EvalEnv::VarDefaultsRaii::VarDefaultsRaii(
    EvalEnv& env, VarDefaults&& var_defaults):
    _env{&env}, _var_set{make_set(var_defaults)} {
    assert(std::none_of(_var_set.begin(), _var_set.end(), [&](const auto ref) {
        return env._var_defaults.count(ref) > 0;
    }));
    env._var_defaults.merge(var_defaults);
}

EvalEnv::VarDefaultsRaii::VarSet
EvalEnv::VarDefaultsRaii::make_set(const VarDefaults& var_defaults) {
    VarSet set;
    for (const auto& [var, res] : var_defaults) {
        assert(_env && _env->_var_defaults.count(var) == 0);
        set.insert(var);
    }
    return set;
}

// EvalEnv

EvalEnv::EvalEnv(Ref<Program> program, eval_flags_t flags):
    EvalBase{program}, _flags{flags}, _program_scope{program} {
    // init global scope
    _scope_stack.push(ScopeStack::Variant{&_program_scope});
    for (auto& mod : program->modules())
        mod->export_symbols(scope());
}

Resolver EvalEnv::resolver(bool in_expr) { return {*this, in_expr}; }

ExprRes EvalEnv::eval(Ref<ast::Block> block) {
    debug() << __FUNCTION__ << "\n";
    try {
        auto num = block->child_num();
        for (unsigned n = 0; n < num; ++n) {
            auto stmt = block->get(n);
            if (n + 1 == num) {
                // if last stmt is an expr, return its result as rvalue
                auto expr_stmt = dynamic_cast<Ref<ast::ExprStmt>>(stmt);
                if (expr_stmt) {
                    auto res = eval_expr(expr_stmt->expr());
                    return {res.type()->deref(), res.move_value().deref()};
                }
            }
            eval_stmt(block->get(n));
        }
    } catch (EvalExceptError& e) {
        // TODO: return status
        throw e;
    }
    return {builtins().type(VoidId), Value{RValue{}}};
}

void EvalEnv::eval_stmt(Ref<ast::Stmt> stmt) {
    EvalVisitor vis{*this};
    return do_eval_stmt(vis, stmt);
}

void EvalEnv::eval_which(Ref<ast::Which> which) {
    EvalWhich ew{*this};
    return do_eval_which(ew, which);
}

ExprRes EvalEnv::eval_expr(Ref<ast::Expr> expr) {
    EvalExprVisitor ev{*this};
    return do_eval_expr(ev, expr);
}

ExprRes EvalEnv::eval_equal(
    Ref<ast::Expr> node,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {
    EvalExprVisitor ev{*this};
    return do_eval_equal(
        ev, node, l_node, std::move(left), r_node, std::move(right));
}

CondRes EvalEnv::eval_cond(Ref<ast::Cond> cond) {
    EvalCond ec{*this};
    return do_eval_cond(ec, cond);
}

ExprRes
EvalEnv::cast(Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl) {
    EvalCast cast{*this};
    return do_cast(cast, node, type, std::move(arg), expl);
}

ExprRes EvalEnv::cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    EvalCast cast{*this};
    return do_cast(cast, node, bi_type_id, std::move(arg), expl);
}

ExprRes EvalEnv::cast_to_idx(Ref<ast::Node> node, ExprRes&& arg) {
    EvalCast cast{*this};
    return do_cast_to_idx(cast, node, std::move(arg));
}

ExprRes EvalEnv::to_boolean(Ref<ast::Expr> expr, ExprRes&& arg, bool expl) {
    // auto boolean = builtins().boolean();
    return cast(expr, BoolId, std::move(arg), expl);
}

bool EvalEnv::init_var(Ref<Var> var, Ref<ast::InitValue> init, bool in_expr) {
    EvalInit ei{*this};
    return do_init_var(ei, var, init, in_expr);
}

bool EvalEnv::init_var_with(Ref<Var> var, ExprRes&& arg) {
    EvalInit ei{*this};
    return do_init_var_with(ei, var, std::move(arg));
}

bool EvalEnv::init_prop(Ref<Prop> prop, Ref<ast::InitValue> init) {
    EvalInit ei{*this};
    return do_init_prop(ei, prop, init);
}

ExprRes
EvalEnv::construct(Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_construct(ef, node, cls, std::move(args));
}

ExprRes
EvalEnv::call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_call(ef, node, std::move(callable), std::move(args));
}

ExprRes EvalEnv::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    EvalFuncall ef{*this};
    return do_funcall(ef, node, fun, std::move(obj), std::move(args));
}

EvalEnv::StackRaii EvalEnv::stack_raii(Ref<Fun> fun, LValue self) {
    return _stack.raii(fun, self);
}

EvalEnv::ScopeRaii EvalEnv::scope_raii(scope_flags_t flags) {
    return scope_raii(scope(), flags);
}

EvalEnv::ScopeRaii EvalEnv::scope_raii(Scope* parent, scope_flags_t flags) {
    assert(!_scope_override);
    return _scope_stack.raii<BasicScope>(parent, flags);
}

EvalEnv::FunScopeRaii
EvalEnv::fun_scope_raii(Ref<Fun> fun, LValue self, scope_flags_t flags) {
    assert(!_scope_override);
    return _scope_stack.raii<FunScope>(fun, self, flags);
}

EvalEnv::AsCondScopeRaii
EvalEnv::as_cond_scope_raii(AsCondContext& as_cond_ctx, scope_flags_t flags) {
    assert(!_scope_override);
    assert(!as_cond_ctx.empty());

    if (as_cond_ctx.is_self()) {
        // (self as A)
        assert(as_cond_ctx.type()->is_class());
        auto cls = as_cond_ctx.type()->as_class();
        auto self = as_cond_ctx.self();
        return _scope_stack.raii<AsCondScope>(cls, self, scope(), flags);
    }

    // (a as A)
    auto var = as_cond_ctx.var();
    var->set_scope_lvl(_scope_stack.size() + 1);
    return _scope_stack.raii<AsCondScope>(var, scope(), flags);
}

EvalEnv::ScopeSwitchRaii EvalEnv::scope_switch_raii(Scope* scope) {
    return {*this, scope};
}

EvalEnv::FlagsRaii EvalEnv::flags_raii(eval_flags_t flags) {
    return {*this, flags};
}

EvalEnv::FlagsRaii EvalEnv::add_flags_raii(eval_flags_t flags_) {
    return {*this, (eval_flags_t)(flags() | flags_)};
}

EvalEnv::FlagsRaii EvalEnv::remove_flags_raii(eval_flags_t flags_) {
    return {*this, (eval_flags_t)(flags() & ~flags_)};
}

EvalEnv::VarDefaultsRaii
EvalEnv::var_defaults_raii(VarDefaults&& var_defaults) {
    return {*this, std::move(var_defaults)};
}

const EvalStack::Item& EvalEnv::stack_top() const {
    assert(!_stack.empty());
    return _stack.top();
}

std::size_t EvalEnv::stack_size() const { return _stack.size(); }

Scope* EvalEnv::scope() {
    return _scope_override ? _scope_override : _scope_stack.top();
}

scope_lvl_t EvalEnv::scope_lvl() const {
    assert(!_scope_override);
    return _scope_stack.size();
}

ExprRes EvalEnv::move_var_default(Ref<Var> var) {
    auto node_h = _var_defaults.extract(var);
    return !node_h.empty() ? std::move(node_h.mapped()) : ExprRes{};
}

void EvalEnv::do_eval_stmt(EvalVisitor& vis, Ref<ast::Stmt> stmt) {
    stmt->accept(vis);
}

void EvalEnv::do_eval_which(EvalWhich& ew, Ref<ast::Which> which) {
    ew.eval_which(which);
}

ExprRes EvalEnv::do_eval_expr(EvalExprVisitor& ev, Ref<ast::Expr> expr) {
    return expr->accept(ev);
}

ExprRes EvalEnv::do_eval_equal(
    EvalExprVisitor& ev,
    Ref<ast::Expr> node,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {
    return ev.binary_op(
        node, Op::Equal, l_node, std::move(left), r_node, std::move(right));
}

CondRes EvalEnv::do_eval_cond(EvalCond& ec, Ref<ast::Cond> cond) {
    return ec.eval_cond(cond);
}

ExprRes EvalEnv::do_cast(
    EvalCast& ec,
    Ref<ast::Node> node,
    Ref<Type> type,
    ExprRes&& arg,
    bool expl) {
    return ec.cast(node, type, std::move(arg), expl);
}

ExprRes EvalEnv::do_cast(
    EvalCast& ec,
    Ref<ast::Node> node,
    BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    return ec.cast(node, bi_type_id, std::move(arg), expl);
}

ExprRes
EvalEnv::do_cast_to_idx(EvalCast& ec, Ref<ast::Node> node, ExprRes&& arg) {
    return ec.cast_to_idx(node, std::move(arg));
}

bool EvalEnv::do_init_var(
    EvalInit& ei, Ref<Var> var, Ref<ast::InitValue> init, bool in_expr) {
    return ei.init_var(var, init, in_expr);
}

bool EvalEnv::do_init_var_with(EvalInit& ei, Ref<Var> var, ExprRes&& arg) {
    return ei.init_var_with(var, std::move(arg));
}

bool EvalEnv::do_init_prop(
    EvalInit& ei, Ref<Prop> prop, Ref<ast::InitValue> init) {
    return ei.init_prop(prop, init);
}

ExprRes EvalEnv::do_construct(
    EvalFuncall& ef, Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args) {
    return ef.construct(node, cls, std::move(args));
}

ExprRes EvalEnv::do_call(
    EvalFuncall& ef,
    Ref<ast::Node> node,
    ExprRes&& callable,
    ExprResList&& args) {
    return ef.call(node, std::move(callable), std::move(args));
}

ExprRes EvalEnv::do_funcall(
    EvalFuncall& ef,
    Ref<ast::Node> node,
    Ref<Fun> fun,
    ExprRes&& obj,
    ExprResList&& args) {
    return ef.funcall(node, fun, std::move(obj), std::move(args));
}

} // namespace ulam::sema

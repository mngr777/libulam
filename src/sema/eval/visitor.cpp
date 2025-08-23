#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/cond.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/eval/which.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <utility>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalVisitor] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

EvalVisitor::EvalVisitor(Ref<Program> program, eval_flags_t flags):
    EvalBase{program}, _program_scope{nullptr, scp::Program}, _flags(flags) {
    // init global scope
    _scope_stack.push(ScopeStack::Variant{&_program_scope});
    for (auto& mod : program->modules())
        mod->export_symbols(scope());
}

ExprRes EvalVisitor::eval(Ref<ast::Block> block) {
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
            block->get(n)->accept(*this);
        }
    } catch (EvalExceptError& e) {
        // TODO: return status
        throw e;
    }
    return {builtins().type(VoidId), Value{RValue{}}};
}

void EvalVisitor::visit(Ref<ast::TypeDef> node) {
    debug() << __FUNCTION__ << " TypeDef\n";
    type_def(node);
}

void EvalVisitor::visit(Ref<ast::VarDefList> node) {
    debug() << __FUNCTION__ << " VarDefList\n";
    auto type_name = node->type_name();
    for (unsigned n = 0; n < node->def_num(); ++n)
        var_def(type_name, node->def(n), node->is_const());
}

void EvalVisitor::visit(Ref<ast::Block> node) {
    debug() << __FUNCTION__ << " Block\n";
    auto scope_raii = _scope_stack.raii<BasicScope>(scope());
    for (unsigned n = 0; n < node->child_num(); ++n)
        node->get(n)->accept(*this);
}

void EvalVisitor::visit(Ref<ast::FunDefBody> node) {
    debug() << __FUNCTION__ << " FunDefBody\n";
    for (unsigned n = 0; n < node->child_num(); ++n)
        node->get(n)->accept(*this);
}

void EvalVisitor::visit(Ref<ast::If> node) {
    debug() << __FUNCTION__ << " If\n";
    assert(node->has_cond());
    assert(node->has_if_branch());

    auto scope_raii = _scope_stack.raii<BasicScope>(scope());
    auto [is_true, ctx] = eval_cond(node->cond());
    if (is_true) {
        node->if_branch()->accept(*this);
    } else if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::For> node) {
    debug() << __FUNCTION__ << " For\n";

    auto scope_raii =
        _scope_stack.raii<BasicScope>(scope(), scp::BreakAndContinue);
    if (node->has_init())
        node->init()->accept(*this);

    unsigned loop_count = 1;
    while (true) {
        if (loop_count++ == 150) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

        {
            auto iter_scope_raii = _scope_stack.raii<BasicScope>(scope());
            AsCondContext as_cond_ctx;
            if (node->has_cond()) {
                auto [is_true, as_cond_ctx] = eval_cond(node->cond());
                if (!is_true)
                    break;
            }

            if (node->has_body()) {
                try {
                    node->body()->accept(*this);
                } catch (const EvalExceptContinue&) {
                    debug() << "continue\n";
                } catch (const EvalExceptBreak&) {
                    debug() << "break\n";
                    break;
                }
            }
        }

        if (node->has_upd())
            node->upd()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::Return> node) {
    debug() << __FUNCTION__ << " Return\n";
    throw EvalExceptReturn(node, ret_res(node));
}

void EvalVisitor::visit(Ref<ast::Break> node) {
    debug() << __FUNCTION__ << " Break\n";
    if (scope()->in(scp::Break)) {
        throw EvalExceptBreak();
    } else {
        diag().error(node, "unexpected break");
    }
}

void EvalVisitor::visit(Ref<ast::Continue> node) {
    debug() << __FUNCTION__ << " Continue\n";
    if (scope()->in(scp::Continue)) {
        throw EvalExceptContinue();
    } else {
        diag().error(node, "unexpected continue");
    }
}

void EvalVisitor::visit(Ref<ast::ExprStmt> node) {
    debug() << __FUNCTION__ << " ExprStmt\n";
    if (node->has_expr())
        eval_expr(node->expr());
}

void EvalVisitor::visit(Ref<ast::EmptyStmt> node) {
    debug() << __FUNCTION__ << " EmptyStmt\n";
}

void EvalVisitor::visit(Ref<ast::While> node) {
    debug() << __FUNCTION__ << " While\n";
    assert(node->has_cond());

    auto scope_raii =
        _scope_stack.raii<BasicScope>(scope(), scp::BreakAndContinue);

    unsigned loop_count = 1;
    while (true) {
        if (loop_count++ == 150) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

        auto iter_scope_raii = _scope_stack.raii<BasicScope>(scope());
        AsCondContext as_cond_ctx;
        if (node->has_cond()) {
            auto [is_true, as_cond_ctx] = eval_cond(node->cond());
            if (!is_true)
                break;
        }

        if (node->has_body()) {
            try {
                node->body()->accept(*this);
            } catch (const EvalExceptContinue&) {
                debug() << "continue\n";
            } catch (const EvalExceptBreak&) {
                debug() << "break\n";
                break;
            }
        }
    }
}

void EvalVisitor::visit(Ref<ast::Which> node) {
    debug() << __FUNCTION__ << "Which\n";
    assert(node->has_expr());

    EvalWhich ew{*this, program(), _scope_stack};
    ew.eval_which(node);
}

void EvalVisitor::visit(Ref<ast::UnaryOp> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::BinaryOp> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::FunCall> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::ArrayAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::MemberAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::TypeOpExpr> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::Ident> node) { eval_expr(node); }

Resolver EvalVisitor::resolver(bool in_expr) {
    return {*this, program(), in_expr};
}

ExprRes EvalVisitor::eval_expr(Ref<ast::Expr> expr) {
    EvalExprVisitor ev{*this, program(), scope()};
    return do_eval_expr(ev, expr);
}

ExprRes EvalVisitor::eval_equal(
    Ref<ast::Expr> node,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {
    EvalExprVisitor ev{*this, program(), scope()};
    return do_eval_equal(
        ev, node, l_node, std::move(left), r_node, std::move(right));
}

ExprRes EvalVisitor::cast(
    Ref<ast::Node> node, Ref<Type> type, ExprRes&& arg, bool expl) {
    EvalCast ec{*this, program(), scope()};
    return do_cast(ec, node, type, std::move(arg), expl);
}

ExprRes EvalVisitor::cast(
    Ref<ast::Node> node, BuiltinTypeId bi_type_id, ExprRes&& arg, bool expl) {
    EvalCast ec{*this, program(), scope()};
    return do_cast(ec, node, bi_type_id, std::move(arg), expl);
}

ExprRes EvalVisitor::to_boolean(Ref<ast::Expr> expr, ExprRes&& arg) {
    auto boolean = builtins().boolean();
    return cast(expr, boolean, std::move(arg));
}

ExprRes EvalVisitor::eval_init(Ref<VarBase> var, Ref<ast::InitValue> init) {
    EvalInit ei{*this, program(), scope()};
    return do_eval_init(ei, var, init);
}

ExprRes
EvalVisitor::construct(Ref<ast::Node> node, Ref<Class> cls, ExprResList&& args) {
    EvalFuncall ef{*this, program(), scope()};
    return do_construct(ef, node, cls, std::move(args));
}

ExprRes
EvalVisitor::call(Ref<ast::Node> node, ExprRes&& callable, ExprResList&& args) {
    EvalFuncall ef{*this, program(), scope()};
    return do_call(ef, node, std::move(callable), std::move(args));
}

ExprRes EvalVisitor::funcall(
    Ref<ast::Node> node, Ref<Fun> fun, ExprRes&& obj, ExprResList&& args) {
    EvalFuncall ef{*this, program(), scope()};
    return do_funcall(ef, node, fun, std::move(obj), std::move(args));
}


ExprRes EvalVisitor::old_funcall(Ref<Fun> fun, LValue self, ExprResList&& args) {
    debug() << __FUNCTION__ << " `" << str(fun->name_id()) << "` {\n";
    assert(fun->params().size() == args.size());

    // push stack "frame"
    auto stack_raii = _stack.raii(fun, self);

    // push fun scope
    scope_lvl_t scope_lvl = _scope_stack.size();
    auto scope_raii =
        _scope_stack.raii<BasicScope>(fun->cls()->scope(), scp::Fun);

    // bind `self`, set `Self`
    scope()->ctx().set_self(self);
    scope()->ctx().set_self_cls(fun->cls());
    if (self.has_auto_scope_lvl())
        self.set_scope_lvl(scope_lvl);

    // bind params
    std::list<Ptr<ast::VarDef>> tmp_defs{};
    std::list<Ptr<Var>> tmp_vars{};
    for (const auto& param : fun->params()) {
        assert(!args.empty());
        auto arg = args.pop_front();

        // binding rvalue or xvalue lvalue to const ref via tmp variable
        if (param->type()->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            // create tmp var def
            auto def = make<ast::VarDef>(param->node()->name());
            auto var = make<Var>(
                param->type_node(), ref(def),
                TypedValue{param->type()->deref(), arg.move_value().deref()},
                param->flags() | Var::TmpFunParam);
            arg = {param->type(), Value{LValue{ref(var)}}};
            tmp_defs.push_back(std::move(def));
            tmp_vars.push_back(std::move(var));
        }
        assert(arg.type()->is_same(param->type()));

        auto var = make<Var>(
            param->type_node(), param->node(), param->type(), param->flags());
        var->set_value(arg.move_value());
        scope()->set(var->name_id(), std::move(var));
    }

    // eval
    try {
        fun->body_node()->accept(*this);
    } catch (EvalExceptReturn& ret) {
        debug() << "}\n";
        return ret.move_res();
    }
    debug() << "}\n";

    if (!fun->ret_type()->is(VoidId)) {
        if (has_flag(evl::NoExec)) {
            if (fun->ret_type()->is_ref()) {
                LValue lval;
                lval.set_is_xvalue(false);
                return {fun->ret_type(), Value{lval}};
            }
            return {fun->ret_type(), Value{RValue{}}};
        } else {
            return {ExprError::NoReturn};
        }
    }
    return {builtins().void_type(), Value{RValue{}}};
}

ExprRes EvalVisitor::do_eval_expr(EvalExprVisitor& ev, Ref<ast::Expr> expr) {
    ExprRes res = expr->accept(ev);
    if (!res)
        throw EvalExceptError("failed to eval expression");
    return res;
}

ExprRes EvalVisitor::do_eval_equal(
    EvalExprVisitor& ev,
    Ref<ast::Expr> node,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {
    return ev.binary_op(
        node, Op::Equal, l_node, std::move(left), r_node, std::move(right));
}

ExprRes EvalVisitor::do_cast(
    EvalCast& ec,
    Ref<ast::Node> node,
    Ref<Type> type,
    ExprRes&& arg,
    bool expl) {
    return ec.cast(node, type, std::move(arg), expl);
}

ExprRes EvalVisitor::do_cast(
    EvalCast& ec,
    Ref<ast::Node> node,
    BuiltinTypeId bi_type_id,
    ExprRes&& arg,
    bool expl) {
    return ec.cast(node, bi_type_id, std::move(arg), expl);
}

ExprRes EvalVisitor::do_eval_init(
    EvalInit& ei, Ref<VarBase> var, Ref<ast::InitValue> init) {
    return ei.eval_init(var, init);
}

CondRes EvalVisitor::eval_cond(Ref<ast::Cond> cond) {
    EvalCond ec{*this, program(), _scope_stack};
    return ec.eval_cond(cond);
}

Ref<AliasType> EvalVisitor::type_def(Ref<ast::TypeDef> node) {
    Ptr<UserType> type = make<AliasType>(str_pool(), builtins(), nullptr, node);
    auto ref = ulam::ref(type);
    if (!resolver(false).resolve(type->as_alias()))
        throw EvalExceptError("failed to resolve type");
    scope()->set(type->name_id(), std::move(type));
    return ref->as_alias();
}

Ref<Var> EvalVisitor::var_def(
    Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const) {
    auto var = make_var(type_name, node, is_const);
    if (!var)
        return {};
    auto ref = ulam::ref(var);
    scope()->set(var->name_id(), std::move(var));
    return ref;
}

Ptr<Var> EvalVisitor::make_var(
    Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const) {
    auto var_flags = is_const ? Var::Const : Var::NoFlags;
    auto var = make<Var>(type_name, node, Ref<Type>{}, var_flags);
    var->set_scope_lvl(_scope_stack.size());
    if (!resolver(false).resolve(ref(var)))
        return {};
    return var;
}

void EvalVisitor::var_init_expr(Ref<Var> var, ExprRes&& init, bool in_expr) {
    assert(init);
    var_init(var, in_expr);
    var->set_value(init.move_value());
}

void EvalVisitor::var_init_default(Ref<Var> var, bool in_expr) {
    var_init(var, in_expr);
    var->set_value(Value{var->type()->construct()});
}

void EvalVisitor::var_init(Ref<Var> var, bool in_expr) {
    assert(var && var->is_ready());
    assert(var->value().empty());
}

ExprRes EvalVisitor::ret_res(Ref<ast::Return> node) {
    ExprRes res;
    if (node->has_expr()) {
        res = eval_expr(node->expr());
    } else {
        res = {builtins().type(VoidId), Value{RValue{}}};
    }

    assert(!_stack.empty());
    auto fun = _stack.top().fun();
    auto ret_type = fun->ret_type();

    // Check if Void fun returns value and vice versa
    if (ret_type->is(VoidId)) {
        if (!res.type()->is(VoidId)) {
            diag().error(node, "return value in function returning Void");
            return {ExprError::NonVoidReturn};
        }
        return res;
    } else if (res.type()->is(VoidId)) {
        diag().error(node, "no return value");
        return {ExprError::NoReturnValue};
    }

    // Check reference
    if (ret_type->is_ref()) {
        if (!res.value().is_lvalue()) {
            diag().error(node, "not a reference");
            return {ExprError::NotReference};
        }
        const LValue lval = res.value().lvalue();
        if (lval.has_scope_lvl() && !lval.has_auto_scope_lvl() &&
            lval.scope_lvl() >= _scope_stack.size()) {
            diag().error(node, "reference to local variable");
            return {ExprError::ReferenceToLocal};
        }
    }

    // Cast
    res = cast(node, ret_type, std::move(res), false);
    return res;
}

Scope* EvalVisitor::scope() { return _scope ? _scope : _scope_stack.top(); }

} // namespace ulam::sema

#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
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
    EvalBase{program, flags}, _program_scope{nullptr, scp::Program} {
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
    auto scope_raii = _scope_stack.raii<BasicScope>(scope(), scp::NoFlags);
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

    if (eval_cond(node->cond())) {
        // if-branch
        node->if_branch()->accept(*this);
    } else {
        // else-branch
        if (node->has_else_branch())
            node->else_branch()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::IfAs> node) {
    debug() << __FUNCTION__ << " IfAs\n";
    assert(node->has_ident());
    assert(node->has_type_name());
    assert(node->has_if_branch());

    // eval ident
    auto res = eval_as_cond_ident(node);

    // resolve type
    auto type = resolve_as_cond_type(node);
    assert(!type->is_ref());

    Ptr<ast::VarDef> def{};
    if (!res.value().empty()) {
        auto dyn_type = res.value().dyn_obj_type(true);
        bool is_match = dyn_type->is_impl_refable_as(type, res.value());
        if (is_match) {
            auto scope_raii = _scope_stack.raii<BasicScope>(scope(), scp::NoFlags);
            auto [var_def, var] = define_as_cond_var(node, std::move(res), type, scope());
            node->if_branch()->accept(*this);
            return;
        }
    }
    if (node->has_else_branch())
        node->else_branch()->accept(*this);
}

void EvalVisitor::visit(Ref<ast::For> node) {
    debug() << __FUNCTION__ << " For\n";

    auto scope_raii =
        _scope_stack.raii<BasicScope>(scope(), scp::BreakAndContinue);

    if (node->has_init())
        node->init()->accept(*this);
    unsigned loop_count = 0;
    while (!node->has_cond() || eval_cond(node->cond())) {
        if (loop_count++ == 100) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

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

    unsigned loop_count = 0;
    while (eval_cond(node->cond())) {
        if (loop_count++ == 100) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

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

    auto scope_raii = _scope_stack.raii<BasicScope>(scope(), scp::Break);
    auto var = make_which_tmp_var(node);
    try {
        bool matched = false;
        for (unsigned n = 0; n < node->case_num(); ++n) {
            // does current case match (or previous fall through)?
            auto case_ = node->case_(n);
            matched = matched || case_->is_default();
            if (!matched) {
                auto is_match =
                    which_match(node->expr(), case_->expr(), ref(var));
                assert(is_match.has_value());
                matched = is_match.value();
            }

            // eval case stmt
            if (matched && case_->has_branch()) {
                case_->branch()->accept(*this);
                break; // implicit break, to fallthru
            }
        }
    } catch (const EvalExceptBreak&) {
        debug() << "break\n";
    }
}

void EvalVisitor::visit(Ref<ast::UnaryOp> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::BinaryOp> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::FunCall> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::ArrayAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::MemberAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::TypeOpExpr> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::Ident> node) { eval_expr(node); }

Ptr<Resolver> EvalVisitor::resolver(bool in_expr, eval_flags_t flags_) {
    return _resolver(in_expr, flags() | flags_);
}

Ptr<EvalExprVisitor>
EvalVisitor::expr_visitor(Scope* scope, eval_flags_t flags_) {
    return _expr_visitor(scope, flags() | flags_);
}

Ptr<EvalInit> EvalVisitor::init_helper(Scope* scope, eval_flags_t flags_) {
    return _init_helper(scope, flags() | flags_);
}

Ptr<EvalCast> EvalVisitor::cast_helper(Scope* scope, eval_flags_t flags_) {
    return _cast_helper(scope, flags() | flags_);
}

Ptr<EvalFuncall>
EvalVisitor::funcall_helper(Scope* scope, eval_flags_t flags_) {
    return _funcall_helper(scope, flags() | flags_);
}

ExprRes EvalVisitor::funcall(Ref<Fun> fun, LValue self, ExprResList&& args) {
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

Ptr<Resolver> EvalVisitor::_resolver(bool in_expr, eval_flags_t flags) {
    return make<Resolver>(*this, program(), in_expr, flags);
}

Ptr<EvalExprVisitor>
EvalVisitor::_expr_visitor(Scope* scope, eval_flags_t flags) {
    return make<EvalExprVisitor>(*this, program(), scope, flags);
}

Ptr<EvalInit> EvalVisitor::_init_helper(Scope* scope, eval_flags_t flags) {
    return make<EvalInit>(*this, program(), scope, flags);
}

Ptr<EvalCast> EvalVisitor::_cast_helper(Scope* scope, eval_flags_t flags) {
    return make<EvalCast>(*this, program(), scope, flags);
}

Ptr<EvalFuncall>
EvalVisitor::_funcall_helper(Scope* scope, eval_flags_t flags) {
    return make<EvalFuncall>(*this, program(), scope, flags);
}

Ref<AliasType> EvalVisitor::type_def(Ref<ast::TypeDef> node) {
    Ptr<UserType> type = make<AliasType>(str_pool(), builtins(), nullptr, node);
    auto ref = ulam::ref(type);
    if (!resolver(false)->resolve(type->as_alias(), scope()))
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
    auto flags = is_const ? Var::Const : Var::NoFlags;
    auto var = make<Var>(type_name, node, Ref<Type>{}, flags);
    var->set_scope_lvl(_scope_stack.size());
    if (!resolver(false)->resolve(ref(var), scope()))
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

Ptr<Var> EvalVisitor::make_which_tmp_var(Ref<ast::Which> node) {
    auto res = eval_which_expr(node);
    return make<Var>(res.move_typed_value());
}

ExprRes EvalVisitor::eval_which_expr(Ref<ast::Which> node) {
    assert(node->has_expr());
    return eval_expr(node->expr());
}

ExprRes EvalVisitor::eval_which_match(
    Ref<ast::Expr> expr,
    Ref<ast::Expr> case_expr,
    ExprRes&& expr_res,
    ExprRes&& case_res) {
    auto res = expr_visitor(scope())->binary_op(
        case_expr, Op::Equal, expr, std::move(expr_res), case_expr,
        std::move(case_res));
    if (!res || (!has_flag(evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to match eval which case");

    // cast to Bool(1) just in case
    return to_boolean(case_expr, std::move(res));
}

std::optional<bool> EvalVisitor::which_match(
    Ref<ast::Expr> expr, Ref<ast::Expr> case_expr, Ref<Var> tmp_var) {
    assert(case_expr);
    ExprRes expr_res{tmp_var->type(), Value{tmp_var->lvalue()}};
    auto ev = expr_visitor(scope());
    auto case_res = case_expr->accept(*ev);
    if (!case_res)
        throw EvalExceptError("failed to eval which case");

    auto res = eval_which_match(
        expr, case_expr, std::move(expr_res), std::move(case_res));
    if (res.value().empty()) {
        assert(has_flag(evl::NoExec));
        return {};
    }

    auto boolean = builtins().boolean();
    assert(res.type() == boolean);
    return boolean->is_true(res.move_value().move_rvalue());
}

ExprRes EvalVisitor::eval_as_cond_ident(Ref<ast::IfAs> node) {
    auto res = eval_expr(node->ident());
    auto arg_type = res.type()->actual();
    if (!arg_type->is_object()) {
        diag().error(node->ident(), "not a class or Atom");
        throw EvalExceptError("if-as var is not an object");
    }
    return res;
}

Ref<Type> EvalVisitor::resolve_as_cond_type(Ref<ast::IfAs> node) {
    auto type = resolver(false)->resolve_type_name(node->type_name(), scope());
    if (!type)
        throw EvalExceptError("failed to resolve type");
    if (!type->is_object()) {
        diag().error(node->type_name(), "not a class or Atom");
        throw EvalExceptError("if-as type is not class or Atom");
    }
    return type;
}

std::pair<Ptr<ast::VarDef>, Ref<Var>> EvalVisitor::define_as_cond_var(
    Ref<ast::IfAs> node, ExprRes&& res, Ref<Type> type, Scope* scope) {
    Ptr<ast::VarDef> def{};
    Ref<Var> ref{};

    LValue lval;
    if (!res.value().empty()) {
        assert(res.value().is_lvalue());
        lval = res.move_value().lvalue().as(type);
    } else {
        if (!has_flag(evl::NoExec)) {
            diag().error(node, "empty value");
            throw EvalExceptError("empty value");
        }
    }

    if (node->ident()->is_self()) {
        scope->ctx().set_self(lval, type->as_class());
    } else {
        def = make<ast::VarDef>(node->ident()->name());
        auto var = make<Var>(
            node->type_name(), ulam::ref(def),
            TypedValue{type->ref_type(), Value{lval}});
        var->set_scope_lvl(_scope_stack.size());
        ref = ulam::ref(var);
        scope->set(var->name_id(), std::move(var));
    }

    return {std::move(def), ref};
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
    auto cast = cast_helper(scope());
    res = cast->cast(node, ret_type, std::move(res), false);
    return res;
}

ExprRes EvalVisitor::eval_expr(Ref<ast::Expr> expr, eval_flags_t flags_) {
    return _eval_expr(expr, flags() | flags_);
}

ExprRes EvalVisitor::_eval_expr(Ref<ast::Expr> expr, eval_flags_t flags) {
    debug() << __FUNCTION__ << "\n";
    ExprRes res = expr->accept(*expr_visitor(scope(), flags));
    if (!res)
        throw EvalExceptError("failed to eval expression");
    return res;
}

bool EvalVisitor::eval_cond(Ref<ast::Expr> expr, eval_flags_t flags_) {
    return _eval_cond(expr, flags() | flags_);
}

bool EvalVisitor::_eval_cond(Ref<ast::Expr> expr, eval_flags_t flags) {
    debug() << __FUNCTION__ << "\n";

    // eval
    auto res = _eval_expr(expr, flags);
    if (!res)
        throw EvalExceptError("failed to eval condition");
    res = to_boolean(expr, std::move(res), flags);
    assert(res);
    return builtins().boolean()->is_true(res.move_value().move_rvalue());
}

ExprRes EvalVisitor::to_boolean(
    Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags_) {
    return _to_boolean(expr, std::move(res), flags() | flags_);
}

ExprRes EvalVisitor::_to_boolean(
    Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags) {
    auto boolean = builtins().boolean();
    auto cast = cast_helper(scope(), flags);
    res = cast->cast(expr, boolean, std::move(res), false);
    if (!res || (!(flags & evl::NoExec) && res.value().empty()))
        throw EvalExceptError("failed to cast to boolean value");
    return std::move(res);
}

} // namespace ulam::sema

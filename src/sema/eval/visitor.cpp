#include "libulam/sema/eval/flags.hpp"
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
    EvalBase{program, flags} {
    // init global scope
    _scope_stack.push(scp::Program);
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
    auto scope_raii{_scope_stack.raii(scp::NoFlags)};
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

    // TODO: check if types are compatible

    Ptr<ast::VarDef> def{};
    if (!res.value().empty()) {
        auto dyn_type = res.value().dyn_obj_type();
        if (dyn_type->is_same(type) || dyn_type->is_impl_castable_to(type)) {
            auto scope_raii{_scope_stack.raii(scp::NoFlags)};
            auto [var_def, var] =
                define_as_cond_var(node, std::move(res), type, scope());
            node->if_branch()->accept(*this);
            return;
        }
    }
    if (node->has_else_branch())
        node->else_branch()->accept(*this);
}

void EvalVisitor::visit(Ref<ast::For> node) {
    debug() << __FUNCTION__ << " For\n";

    auto scope_raii{_scope_stack.raii(scp::Break | scp::Continue)};
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

    auto scope_raii{_scope_stack.raii(scp::Break | scp::Continue)};
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

    auto scope_raii{_scope_stack.raii(scp::Break)};
    bool matched = false;

    // eval switch expr, store result in tmp var
    auto ev = expr_visitor(scope());
    Ptr<Var> var{};
    {
        ExprRes res = node->expr()->accept(*ev);
        if (!res)
            throw EvalExceptError("cannot eval which expr");
        var = make<Var>(res.move_typed_value());
    }

    try {
        for (unsigned n = 0; n < node->case_num(); ++n) {
            // does current case match (or previous fall through)?
            auto case_ = node->case_(n);
            matched = matched || case_->is_default();
            if (!matched) {
                auto [is_match, ok] =
                    ev->match(node->expr(), ref(var), case_->expr());
                if (!ok)
                    throw EvalExceptError("cannot eval case expr");
                matched = is_match;
            }

            // eval case stmt
            if (matched && case_->has_branch())
                case_->branch()->accept(*this);
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

Ptr<Resolver> EvalVisitor::resolver(eval_flags_t flags_) {
    return _resolver(flags() | flags_);
}

Ptr<EvalExprVisitor>
EvalVisitor::expr_visitor(Ref<Scope> scope, eval_flags_t flags_) {
    return _expr_visitor(scope, flags() | flags_);
}

Ptr<EvalInit> EvalVisitor::init_helper(Ref<Scope> scope, eval_flags_t flags_) {
    return _init_helper(scope, flags() | flags_);
}

Ptr<EvalCast> EvalVisitor::cast_helper(Ref<Scope> scope, eval_flags_t flags_) {
    return _cast_helper(scope, flags() | flags_);
}

Ptr<EvalFuncall>
EvalVisitor::funcall_helper(Ref<Scope> scope, eval_flags_t flags_) {
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
        _scope_stack.raii(make<BasicScope>(fun->cls()->scope(), scp::Fun));

    // bind `self`, set `Self`
    scope()->set_self(self);
    scope()->set_self_cls(fun->cls());
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
    if (!fun->ret_type()->is(VoidId))
        return {ExprError::NoReturn};
    return {builtins().void_type(), Value{RValue{}}};
}

Ptr<Resolver> EvalVisitor::_resolver(eval_flags_t flags) {
    return make<Resolver>(*this, program(), flags);
}

Ptr<EvalExprVisitor>
EvalVisitor::_expr_visitor(Ref<Scope> scope, eval_flags_t flags) {
    return make<EvalExprVisitor>(*this, program(), scope, flags);
}

Ptr<EvalInit> EvalVisitor::_init_helper(Ref<Scope> scope, eval_flags_t flags) {
    return make<EvalInit>(*this, program(), scope, flags);
}

Ptr<EvalCast> EvalVisitor::_cast_helper(Ref<Scope> scope, eval_flags_t flags) {
    return make<EvalCast>(*this, program(), scope, flags);
}

Ptr<EvalFuncall>
EvalVisitor::_funcall_helper(Ref<Scope> scope, eval_flags_t flags) {
    return make<EvalFuncall>(*this, program(), scope, flags);
}

Ref<AliasType> EvalVisitor::type_def(Ref<ast::TypeDef> node) {
    Ptr<UserType> type = make<AliasType>(str_pool(), builtins(), nullptr, node);
    auto ref = ulam::ref(type);
    if (!resolver()->resolve(type->as_alias(), scope()))
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
    if (!resolver()->resolve(ref(var), scope()))
        return {};
    return var;
}

void EvalVisitor::var_init_expr(Ref<Var> var, ExprRes&& init) {
    assert(init);
    var_init(var);
    var->set_value(init.move_value());
}

void EvalVisitor::var_init_default(Ref<Var> var) {
    var_init(var);
    var->set_value(Value{var->type()->construct()});
}

void EvalVisitor::var_init(Ref<Var> var) {
    assert(var && var->is_ready());
    assert(var->value().empty());
}

ExprRes EvalVisitor::eval_as_cond_ident(Ref<ast::IfAs> node) {
    ExprRes res = node->ident()->accept(*expr_visitor(scope()));
    if (!res)
        throw EvalExceptError("failed to eval if-as ident");
    auto arg_type = res.type()->actual();
    if (!arg_type->is_object()) {
        diag().error(node->ident(), "not a class or Atom");
        throw EvalExceptError("if-as var is not an object");
    }
    return res;
}

Ref<Type> EvalVisitor::resolve_as_cond_type(Ref<ast::IfAs> node) {
    auto type = resolver()->resolve_type_name(node->type_name(), scope());
    if (!type)
        throw EvalExceptError("failed to resolve type");
    if (!type->is_object()) {
        diag().error(node->type_name(), "not a class or Atom");
        throw EvalExceptError("if-as type is not class or Atom");
    }
    return type;
}

std::pair<Ptr<ast::VarDef>, Ref<Var>> EvalVisitor::define_as_cond_var(
    Ref<ast::IfAs> node, ExprRes&& res, Ref<Type> type, Ref<Scope> scope) {
    Ptr<ast::VarDef> def{};
    Ref<Var> ref{};

    LValue lval;
    if (!res.value().empty()) {
        assert(res.value().is_lvalue());
        lval = res.move_value().as(type->deref());
    } else {
        if (!has_flag(evl::NoExec)) {
            diag().error(node, "empty value");
            throw EvalExceptError("empty value");
        }
    }

    if (node->ident()->is_self()) {
        scope->set_self(lval, type->as_class());
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
        return false;

    // cast to Bool(1)
    auto boolean = builtins().boolean();
    auto cast = cast_helper(scope(), flags);
    res = cast->cast(expr, boolean, std::move(res), true);
    if (!res)
        return false;
    if (!(flags & evl::NoExec) && res.value().empty())
        throw EvalExceptError("empty value in condition");
    return boolean->is_true(res.move_value().move_rvalue());
}

} // namespace ulam::sema

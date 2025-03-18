#include <cassert>
#include <exception>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

#define DEBUG_EVAL // TEST
#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalVisitor] "
#    include "src/debug.hpp"
#endif

namespace ulam::sema {

EvalVisitor::EvalVisitor(Ref<Program> program):
    _program{program}, _resolver{program} {
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
                // if last stmt is an expr, return its result
                auto expr_stmt = dynamic_cast<Ref<ast::ExprStmt>>(stmt);
                if (expr_stmt)
                    return eval_expr(expr_stmt->expr());
            }
            block->get(n)->accept(*this);
        }
    } catch (EvalExceptError& e) {
        // TODO: return status
        throw e;
    }
    return {_program->builtins().type(VoidId), Value{RValue{}}};
}

void EvalVisitor::visit(Ref<ast::TypeDef> node) {
    debug() << __FUNCTION__ << " TypeDef\n";
    Ptr<UserType> type = make<AliasType>(builtins(), nullptr, node);
    if (_resolver.resolve(type->as_alias(), scope()))
        scope()->set(type->name_id(), std::move(type));
}

void EvalVisitor::visit(Ref<ast::VarDefList> node) {
    debug() << __FUNCTION__ << " VarDefList\n";
    auto type_name = node->type_name();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def_node = node->def(n);
        auto var = make<Var>(type_name, def_node, Ref<Type>{}, Var::NoFlags);
        var->set_scope_lvl(_scope_stack.size());
        if (!_resolver.resolve(ref(var), scope()))
            continue;
        if (var->value().empty()) {
            if (def_node->has_init()) {
                EvalExprVisitor ev{*this, scope()};
                auto [val, ok] = ev.eval_init(def_node, var->type());
                if (!ok)
                    throw EvalExceptError("failed to eval init value");
                var->set_value(std::move(val));
            } else {
                auto rval = var->type()->construct();
                var->set_value(Value{std::move(rval)});
            }
        }
        scope()->set(var->name_id(), std::move(var));
    }
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
    EvalExprVisitor ev{*this, scope()};
    ExprRes res = node->ident()->accept(ev);
    if (!res)
        throw EvalExceptError("failed to eval if-as ident");
    auto arg_type = res.type()->actual();
    if (!arg_type->is_object()) {
        diag().error(node->ident(), "not a class or Atom");
        throw EvalExceptError("if-as var is not an object");
    }

    // resolve type
    Resolver resolver{_program};
    auto type = resolver.resolve_type_name(node->type_name(), scope());
    if (!type)
        throw std::exception();
    type = type->canon();
    if (!type->is_object()) {
        diag().error(node->type_name(), "not a class or Atom");
        throw EvalExceptError("if-as type is not class or Atom");
    }

    Ref<Type> dyn_type = res.type();
    if (!res.value().empty())
        dyn_type = res.value().dyn_obj_type()->actual();
    if (dyn_type->is_same(type) || dyn_type->is_impl_castable_to(type)) {
        auto scope_raii{_scope_stack.raii(scp::NoFlags)};
        auto val = res.move_value();
        assert(val.is_lvalue());
        if (node->ident()->is_self()) {
            scope()->set_self(val.lvalue().self().as(type), type->as_class());
        } else {
            auto obj_view = val.lvalue().as(type);
            auto def = make<ast::VarDef>(node->ident()->name());
            auto var = make<Var>(
                node->type_name(), ref(def),
                TypedValue{type->ref_type(), Value{LValue{obj_view}}});
            var->set_scope_lvl(_scope_stack.size());
            scope()->set(var->name_id(), std::move(var));
        }

        node->if_branch()->accept(*this);

    } else if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
    }
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
    ExprRes res;
    if (node->has_expr()) {
        res = eval_expr(node->expr());
    } else {
        res = {_program->builtins().type(VoidId), Value{RValue{}}};
    }
    throw EvalExceptReturn(node, std::move(res));
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
    EvalExprVisitor ev{*this, scope()};
    Ptr<Var> var{};
    {
        ExprRes res = node->expr()->accept(ev);
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
                    ev.match(node->expr(), ref(var), case_->expr());
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

ExprRes EvalVisitor::funcall(Ref<Fun> fun, LValue self, TypedValueList&& args) {
    debug() << __FUNCTION__ << " `" << str(fun->name_id()) << "` {\n";
    assert(fun->params().size() == args.size());

    // push fun scope
    scope_lvl_t scope_lvl = _scope_stack.size();
    auto sr_params =
        _scope_stack.raii(make<BasicScope>(fun->cls()->scope(), scp::Fun));

    // bind `self`, set `Self`
    // scope()->set_self(self.as(fun->cls()));
    scope()->set_self(self);
    scope()->set_self_cls(fun->cls());
    if (self.has_auto_scope_lvl())
        self.set_scope_lvl(scope_lvl);

    // bind params
    std::list<Ptr<ast::VarDef>> tmp_var_defs{};
    std::list<Ptr<Var>> tmp_vars{};
    for (const auto& param : fun->params()) {
        assert(!args.empty());

        auto arg = std::move(args.front());
        args.pop_front();

        // binding rvalue or xvalue lvalue to const ref via tmp variable
        if (param->type()->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            // create tmp vardef
            auto var_def = make<ast::VarDef>(param->node()->name());
            auto var = make<Var>(
                param->type_node(), ref(var_def),
                TypedValue{param->type()->deref(), arg.move_value().deref()},
                param->flags() | Var::TmpFunParam);
            arg = {param->type(), Value{LValue{ref(var)}}};
            tmp_var_defs.push_back(std::move(var_def));
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

        ExprRes res = ret.move_res();
        auto ret_type = fun->ret_type();
        if (!res)
            return res;

        auto type = res.type();
        // auto val = res.move_value();

        // Void ret type
        if (ret_type->is(VoidId)) {
            if (!res.type()->is(VoidId)) {
                diag().error(
                    ret.node(), "return value in function returning Void");
                return {ExprError::NonVoidReturn};
            }
            return {ret_type, Value{RValue{}}};

        } else if (type->is(VoidId)) {
            diag().error(ret.node(), "no return value");
            return {ExprError::NoReturnValue};
        }

        // Reference ret type
        if (ret_type->is_ref()) {
            if (!res.value().is_lvalue()) {
                diag().error(ret.node(), "not a reference");
                return {ExprError::NotReference};
            }
            // TODO: test
            const LValue lval = res.value().lvalue();
            if (lval.has_scope_lvl() && !lval.has_auto_scope_lvl() &&
                lval.scope_lvl() >= _scope_stack.size()) {
                diag().error(ret.node(), "reference to local");
                return {ExprError::ReferenceToLocal};
            }
        }
        EvalExprVisitor ev{*this, scope()};
        res = ev.cast(ret.node()->expr(), ret_type, std::move(res), false);
        return res;
    }
    debug() << "}\n";
    if (!fun->ret_type()->is(VoidId))
        return {ExprError::NoReturn};
    return {fun->ret_type(), Value{RValue{}}};
}

ExprRes EvalVisitor::eval_expr(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n";
    EvalExprVisitor ev{*this, scope()};
    ExprRes res = expr->accept(ev);
    if (!res)
        throw EvalExceptError("failed to eval expression");
    return res;
}

bool EvalVisitor::eval_cond(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n";
    EvalExprVisitor ev{*this, scope()};
    auto res = ev.eval_cond(expr);
    if (!res)
        throw EvalExceptError("failed to eval condition");

    auto boolean = _program->builtins().boolean();
    assert(res.type() == boolean);
    return boolean->is_true(res.move_value().move_rvalue());
}

} // namespace ulam::sema

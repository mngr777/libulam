#include <cassert>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <utility>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalVisitor] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

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
    auto sr = env().scope_raii();
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

    auto sr = env().scope_raii();
    auto [is_true, ctx] = env().eval_cond(node->cond());
    if (is_true) {
        node->if_branch()->accept(*this);
    } else if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::For> node) {
    debug() << __FUNCTION__ << " For\n";

    auto sr = env().scope_raii(scp::BreakAndContinue);
    if (node->has_init())
        node->init()->accept(*this);

    unsigned loop_count = 1;
    while (true) {
        if (loop_count++ == 150) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

        {
            auto iter_sr = env().scope_raii();
            AsCondContext as_cond_ctx;
            if (node->has_cond()) {
                auto [is_true, as_cond_ctx] = env().eval_cond(node->cond());
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
        env().eval_expr(node->expr());
}

void EvalVisitor::visit(Ref<ast::EmptyStmt> node) {
    debug() << __FUNCTION__ << " EmptyStmt\n";
}

void EvalVisitor::visit(Ref<ast::While> node) {
    debug() << __FUNCTION__ << " While\n";
    assert(node->has_cond());

    auto sr = env().scope_raii(scp::BreakAndContinue);
    unsigned loop_count = 1;
    while (true) {
        if (loop_count++ == 150) // TODO: max loops option
            throw EvalExceptError("for loop limit exceeded");

        auto iter_sr = env().scope_raii();
        AsCondContext as_cond_ctx;
        if (node->has_cond()) {
            auto [is_true, as_cond_ctx] = env().eval_cond(node->cond());
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
    return env().eval_which(node);
}

void EvalVisitor::visit(Ref<ast::UnaryOp> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::BinaryOp> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::FunCall> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::ArrayAccess> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::MemberAccess> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::TypeOpExpr> node) { env().eval_expr(node); }

void EvalVisitor::visit(Ref<ast::Ident> node) { env().eval_expr(node); }

Ref<AliasType> EvalVisitor::type_def(Ref<ast::TypeDef> node) {
    Ptr<UserType> type = make<AliasType>(str_pool(), builtins(), nullptr, node);
    auto ref = ulam::ref(type);
    if (!env().resolver(false).resolve(type->as_alias()))
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
    var->set_scope_lvl(env().stack_size());
    if (!env().resolver(false).resolve(ref(var)))
        return {};
    return var;
}

ExprRes EvalVisitor::ret_res(Ref<ast::Return> node) {
    ExprRes res;
    if (node->has_expr()) {
        res = env().eval_expr(node->expr());
    } else {
        res = {builtins().type(VoidId), Value{RValue{}}};
    }

    auto fun = env().stack_top().fun();
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
            lval.scope_lvl() >= env().stack_size()) {
            diag().error(node, "reference to local variable");
            return {ExprError::ReferenceToLocal};
        }
    }

    // Cast
    res = env().cast(node, ret_type, std::move(res), false);
    return res;
}

} // namespace ulam::sema

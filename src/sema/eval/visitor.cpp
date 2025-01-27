#include "libulam/ast/nodes/module.hpp"
#include "libulam/semantic/scope.hpp"
#include <cassert>
#include <exception>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam::sema {

EvalVisitor::EvalVisitor(Ref<Program> program):
    _program{program}, _resolver{program} {
    // init global scope
    _scope_stack.push(scp::Program);
    for (auto& mod : program->modules())
        mod->export_symbols(scope());
}

ExprRes EvalVisitor::eval(Ref<ast::Block> block) {
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
        // TODO
    }
    return {_program->builtins().type(VoidId), RValue{}};
}

void EvalVisitor::visit(Ref<ast::TypeDef> node) {
    Ptr<UserType> type = make<AliasType>(nullptr, node);
    if (_resolver.resolve(type->as_alias(), scope()))
        scope()->set(type->name_id(), std::move(type));
}

void EvalVisitor::visit(Ref<ast::VarDefList> node) {
    auto type_name = node->type_name();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def_node = node->def(n);
        auto var = make<Var>(type_name, def_node, Ref<Type>{}, Var::NoFlags);
        if (_resolver.resolve(ref(var), scope())) {
            scope()->set(var->name_id(), std::move(var));
        }
    }
}

void EvalVisitor::visit(Ref<ast::Block> node) {
    auto scope_raii{_scope_stack.raii(scp::NoFlags)};
    for (unsigned n = 0; n < node->child_num(); ++n)
        node->get(n)->accept(*this);
}

void EvalVisitor::visit(Ref<ast::FunDefBody> node) {
    for (unsigned n = 0; n < node->child_num(); ++n)
        node->get(n)->accept(*this);
}

void EvalVisitor::visit(Ref<ast::If> node) {
    assert(node->has_cond());
    if (eval_cond(node->cond())) {
        // if-branch
        if (node->has_if_branch())
            node->if_branch()->accept(*this);
    } else {
        // else-branch
        if (node->has_else_branch())
            node->else_branch()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::For> node) {
    auto scope_raii{_scope_stack.raii(scp::Break | scp::Continue)};
    if (node->has_init())
        node->init()->accept(*this);

    unsigned loop_count = 0;
    while (!node->has_cond() || eval_cond(node->cond())) {
        if (loop_count++ == 1000) // TODO: max loops option
            throw std::exception();

        if (node->has_body())
            node->body()->accept(*this);
        if (node->has_upd())
            node->upd()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::Return> node) {
    ExprRes res;
    if (node->has_expr()) {
        res = eval_expr(node->expr());
    } else {
        res = {_program->builtins().type(VoidId), RValue{}};
    }
    throw EvalExceptReturn(std::move(res));
}

void EvalVisitor::visit(Ref<ast::ExprStmt> node) {
    if (node->has_expr())
        eval_expr(node->expr());
}

void EvalVisitor::visit(Ref<ast::While> node) {
    assert(node->has_cond());
    unsigned loop_count = 0;
    while (eval_cond(node->cond())) {
        if (loop_count++ == 1000) // TODO: max loops option
            throw std::exception();

        if (node->has_body())
            node->body()->accept(*this);
    }
}

void EvalVisitor::visit(Ref<ast::FunCall> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::ArrayAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::MemberAccess> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::TypeOpExpr> node) { eval_expr(node); }

void EvalVisitor::visit(Ref<ast::Ident> node) { eval_expr(node); }

ExprRes EvalVisitor::funcall(Ref<Fun> fun, TypedValueList&& args) {
    assert(fun->params().size() == args.size());

    // add scope
    _scope_stack.raii(make<BasicScope>(fun->cls()->scope(), scp::Fun));

    // bind params
    for (const auto& param : fun->params()) {
        assert(args.size() >= 0);
        auto tv = std::move(args.front());
        args.pop_front();
        assert(param->type() && tv.type() == param->type());
        auto var = make<Var>(
            param->type_node(), param->node(), tv.type(), param->flags());
        scope()->set(var->name_id(), std::move(var));
    }

    // eval
    try {
        fun->body_node()->accept(*this);
    } catch (EvalExceptReturn& ret) {
        return ret.move_res();
    }
    return {_program->builtins().type(VoidId), RValue{}};
}

ExprRes EvalVisitor::eval_expr(Ref<ast::Expr> expr) {
    ExprVisitor ev(_program, scope());
    ExprRes res = expr->accept(ev);
    if (!res.ok())
        throw std::exception(); // TODO
    return res;
}

bool EvalVisitor::eval_cond(Ref<ast::Expr> expr) {
    ExprRes res{eval_expr(expr)};
    // res = _cast.to_boolean(std::move(res), expr, true /* implicit */);
    // if (!res.ok())
    //     throw std::exception();
    // return res.value().rvalue()->get<Bool>();
    // TODO
    return false;
}

} // namespace ulam::sema

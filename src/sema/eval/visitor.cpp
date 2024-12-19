#include <exception>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/var.hpp>
#include <stdexcept>
#include <utility>

namespace ulam::sema {

EvalVisitor::EvalVisitor(Ref<Program> program):
    _program{program}, _resolver{program}, _cast{program} {
    // init global scope
    auto scope_raii{_scope_stack.raii(scp::Program)};
    for (auto& mod : program->modules())
        mod->export_symbols(scope());
}

ExprRes EvalVisitor::eval(Ref<ast::Block> block) {
    try {
        for (unsigned n = 0; n < block->child_num(); ++n)
            block->get(n)->accept(*this);
    } catch (std::exception&) {
        return {ExprError::NotImplemented};
    }
    return {ExprError::NotImplemented}; // TODO
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
        if (_resolver.resolve(ref(var), scope()))
            scope()->set(var->name_id(), std::move(var));
    }
}

void EvalVisitor::visit(Ref<ast::Block> block) {
    auto scope_raii{_scope_stack.raii(scp::NoFlags)};
    for (unsigned n = 0; n < block->child_num(); ++n)
        block->get(n)->accept(*this);
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
    ExprVisitor ev(_program, scope());
    while (!node->has_cond() || eval_cond(node->cond())) {
        if (loop_count++ == 1000) // TODO: max loops option
            throw std::exception();

        if (node->has_body())
            node->body()->accept(*this);
        if (node->has_upd())
            node->upd()->accept(*this);
    }
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

ExprRes EvalVisitor::eval_expr(Ref<ast::Expr> expr) {
    ExprVisitor ev(_program, scope());
    ExprRes res = expr->accept(ev);
    if (!res.ok())
        throw std::exception(); // TODO
    return res;
}

bool EvalVisitor::eval_cond(Ref<ast::Expr> expr) {
    ExprRes res{eval_expr(expr)};
    res = _cast.to_boolean(std::move(res), expr, true /* implicit */);
    if (!res.ok())
        throw std::exception();
    return res.value().rvalue()->get<Bool>();
}

} // namespace ulam::sema

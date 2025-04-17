#include "./visitor.hpp"
#include "../out.hpp"
#include "./cast.hpp"
#include "./expr_res.hpp"
#include "./expr_visitor.hpp"
#include "./flags.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "libulam/sema/eval/flags.hpp"
#include "libulam/sema/eval/visitor.hpp"
#include "libulam/semantic/scope/flags.hpp"
#include <libulam/sema/eval/except.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

// TODO: throw when missing data

void EvalVisitor::visit(ulam::Ref<ulam::ast::Block> node) {
    std::size_t size{0};
    if (codegen_enabled()) {
        append("{");
        size = _data.size();
    }
    ulam::sema::EvalVisitor::visit(node);
    if (codegen_enabled()) {
        bool nospace = _data.size() == size;
        append("}", nospace);
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::If> node) {
    // codegen
    if (codegen_enabled()) {
        auto no_exec_raii = flags_raii(flags() | ulam::sema::evl::NoExec);
        append("{");

        // cond
        auto cond_res = eval_expr(node->cond());
        if (cond_res.has_data()) {
            append(cond_res.data<std::string>());
            append("cond");
        }

        // if-branch
        node->if_branch()->accept(*this);
        append("if");

        // else-branch
        if (node->has_else_branch()) {
            node->else_branch()->accept(*this);
            append("else");
        }

        append("}");
    }

    // exec
    auto no_codegen_raii = flags_raii(flags() | evl::NoCodegen);
    ulam::sema::EvalVisitor::visit(node);
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::While> node) {
    // codegen
    if (codegen_enabled()) {
        auto no_exec_raii = flags_raii(flags() | ulam::sema::evl::NoExec);
        auto scope_raii =
            _scope_stack.raii(ulam::scp::Break | ulam::scp::Continue);
        append("{");

        // cond, TODO: cast to Bool
        auto cond_res = eval_expr(node->cond());
        if (cond_res.has_data()) {
            append(cond_res.data<std::string>());
            append("cond");
        }

        // body
        node->body()->accept(*this);

        append("_" + std::to_string(next_loop_idx()) + ": while");
        append("}");
    }

    // exec
    auto no_codegen_raii = flags_raii(flags() | evl::NoCodegen);
    ulam::sema::EvalVisitor::visit(node);
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::For> node) {
    // codegen
    if (codegen_enabled()) {
        auto no_exec_raii = flags_raii(flags() | ulam::sema::evl::NoExec);
        auto scope_raii =
            _scope_stack.raii(ulam::scp::Break | ulam::scp::Continue);
        append("{");

        // init
        if (node->has_init())
            node->init()->accept(*this);

        // cond, TODO: cast to Bool
        if (node->has_cond()) {
            auto cond_res = eval_expr(node->cond());
            if (cond_res.has_data())
                append(cond_res.data<std::string>());
        }
        append("cond");

        // body
        node->body()->accept(*this);

        append("_" + std::to_string(next_loop_idx()) + ":");
        if (node->has_upd()) {
            auto upd_res = eval_expr(node->upd());
            if (upd_res.has_data())
                append(upd_res.data<std::string>());
        }
        append("while");
        append("}");
    }

    // exec
    auto no_codegen_raii = flags_raii(flags() | evl::NoCodegen);
    ulam::sema::EvalVisitor::visit(node);
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Return> node) {
    auto res = ret_res(node);
    if (codegen_enabled()) {
        if (res.has_data()) {
            append(res.data<std::string>());
            append("return");
        }
    }
    // TODO: check scope, type
    if (!has_flag(ulam::sema::evl::NoExec))
        throw ulam::sema::EvalExceptReturn(node, std::move(res));
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::ExprStmt> node) {
    if (!node->has_expr())
        return;
    auto res = eval_expr(node->expr());
    if (codegen_enabled())
        append(exp::data(res));
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::EmptyStmt> node) {
    if (codegen_enabled())
        append(";");
}

ulam::Ptr<ulam::sema::EvalExprVisitor> EvalVisitor::_expr_visitor(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalExprVisitor>(
        *this, program(), _stringifier, scope, flags);
}

ulam::Ptr<ulam::sema::EvalInit> EvalVisitor::_init_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalInit>(*this, program(), scope, flags);
}

ulam::Ptr<ulam::sema::EvalCast> EvalVisitor::_cast_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalCast>(*this, program(), scope, flags);
}

ulam::Ptr<ulam::sema::EvalFuncall> EvalVisitor::_funcall_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalFuncall>(*this, program(), scope, flags);
}

ulam::Ref<ulam::AliasType>
EvalVisitor::type_def(ulam::Ref<ulam::ast::TypeDef> node) {
    auto alias_type = ulam::sema::EvalVisitor::type_def(node);
    if (alias_type && codegen_enabled())
        append(out::type_def_str(alias_type) + "; ");
    return alias_type;
}

void EvalVisitor::var_init_expr(
    ulam::Ref<ulam::Var> var, ulam::sema::ExprRes&& init) {
    auto data = exp::data(init);
    ulam::sema::EvalVisitor::var_init_expr(var, std::move(init));
    if (codegen_enabled()) {
        append("=");
        append(data);
        append("; ", true);
    }
}

void EvalVisitor::var_init_default(ulam::Ref<ulam::Var> var) {
    ulam::sema::EvalVisitor::var_init_default(var);
    if (codegen_enabled())
        append("; ", true);
}

void EvalVisitor::var_init(ulam::Ref<ulam::Var> var) {
    if (codegen_enabled())
        append(out::var_def_str(str_pool(), var));
}

ulam::sema::ExprRes EvalVisitor::_eval_expr(
    ulam::Ref<ulam::ast::Expr> expr, ulam::sema::eval_flags_t flags) {
    auto res = ulam::sema::EvalVisitor::_eval_expr(expr, flags);
    assert(res);
    debug() << "expr: "
            << (res.has_data() ? res.data<std::string>()
                               : std::string{"no data"})
            << "\n";
    return res;
}

void EvalVisitor::append(std::string data, bool nospace) {
    if (!nospace && !_data.empty())
        _data += " ";
    _data += std::move(data);
}

#include "./visitor.hpp"
#include "../out.hpp"
#include "./cast.hpp"
#include "./codegen.hpp"
#include "./expr_res.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "./codegen/context_stack.hpp"
#include <libulam/sema/eval/except.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

void EvalVisitor::visit(ulam::Ref<ulam::ast::Block> node) {
    bool has_pref = false;
    std::size_t size{0};
    if (codegen_enabled()) {
        has_pref = gen().has_next_prefix();
        gen().block_open();
        size = gen().code().size();
    }
    Base::visit(node);
    if (codegen_enabled()) {
        // no space before closing empty block
        bool nospace = !has_pref && (gen().code().size() == size);
        gen().block_close(nospace);
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::If> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    gen().block_open();

    // cond
    {
        auto sr = env().scope_raii();
        auto cond = node->cond();
        auto cond_res = env().eval_cond(cond);

        // if-branch
        maybe_wrap_stmt(node->if_branch(), cond->is_as_cond());
        gen().append("if");
    }

    // else-branch
    if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
        gen().append("else");
    }

    gen().block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::For> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto cr = gen().ctx_stack().raii(gen::ForContext{});
    auto sr = env().scope_raii(ulam::scp::BreakAndContinue);
    gen().block_open();

    // init
    if (!node->init()->is_empty())
        node->init()->accept(*this);

    // cond
    {
        auto iter_sr = env().scope_raii();
        auto cond = node->cond();
        auto cond_res = env().eval_cond(cond);

        // body
        if (node->has_body())
            maybe_wrap_stmt(node->body(), cond->is_as_cond());
    }

    gen().append("_" + gen().next_tmp_idx_str() + ":");
    if (node->has_upd()) {
        auto upd_res = env().eval_expr(node->upd());
        if (upd_res.has_data())
            gen().append(exp::data(upd_res));
    }
    gen().append("while");
    gen().block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::While> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto tmp_idx = std::to_string(gen().next_tmp_idx());
    auto cr = gen().ctx_stack().raii(gen::WhileContext{});
    auto sr = env().scope_raii(ulam::scp::BreakAndContinue);
    gen().block_open();

    auto cond_res = env().eval_cond(node->cond());

    // body
    node->body()->accept(*this);

    gen().append("_" + tmp_idx + ": while");
    gen().block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Return> node) {
    if (!codegen_enabled())
        return Base::visit(node);

    auto res = ret_res(node);
    if (res.has_data()) {
        gen().append(res.data<std::string>());
        gen().append("return");
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Break> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }
    if (gen().ctx_stack().top_is<gen::WhichContext>())
        gen().ctx_stack().top<gen::WhichContext>().set_case_has_breaks(true);
    gen().append("break");
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Continue> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }
    gen().append("goto");
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::ExprStmt> node) {
    if (!node->has_expr())
        return;
    auto res = env().eval_expr(node->expr());
    if (codegen_enabled())
        gen().append(exp::data(res));
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::EmptyStmt> node) {
    if (codegen_enabled())
        gen().append(";");
}

ulam::Ref<ulam::AliasType>
EvalVisitor::type_def(ulam::Ref<ulam::ast::TypeDef> node) {
    auto alias_type = Base::type_def(node);
    if (alias_type && codegen_enabled()) {
        auto strf = gen().make_strf();
        gen().append(out::type_def_str(strf, alias_type) + "; ");
    }
    return alias_type;
}

ulam::Ptr<ulam::Var> EvalVisitor::make_var(
    ulam::Ref<ulam::ast::TypeName> type_name,
    ulam::Ref<ulam::ast::VarDef> node,
    bool is_const) {
    // EvalEnv::FlagsRaii fr{};
    // if (is_const)
    //     fr = env().add_flags_raii(evl::NoConstFold);
    return Base::make_var(type_name, node, is_const);
}

void EvalVisitor::maybe_wrap_stmt(ulam::Ref<ulam::ast::Stmt> stmt, bool wrap) {
    bool needs_wrap = wrap && !stmt->is_block();

    if (needs_wrap)
        gen().block_open();

    stmt->accept(*this);

    if (needs_wrap)
        gen().block_close();
}

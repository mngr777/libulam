#pragma once
#include "./context_stack.hpp"
#include "./stringifier.hpp"
#include "libulam/ast/nodes/exprs.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <string>

class EvalVisitor : public ulam::sema::EvalVisitor {
public:
    using Base = ulam::sema::EvalVisitor;

    explicit EvalVisitor(
        ulam::Ref<ulam::Program> program,
        ulam::sema::eval_flags_t flags = ulam::sema::evl::NoFlags):
        ulam::sema::EvalVisitor{program, flags} {}

    ulam::sema::ExprRes eval(ulam::Ref<ulam::ast::Block> block) override;

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::If> node) override;
    void visit(ulam::Ref<ulam::ast::For> node) override;
    void visit(ulam::Ref<ulam::ast::While> node) override;
    void visit(ulam::Ref<ulam::ast::Which> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
    void visit(ulam::Ref<ulam::ast::Break> node) override;
    void visit(ulam::Ref<ulam::ast::Continue> node) override;
    void visit(ulam::Ref<ulam::ast::ExprStmt> node) override;
    void visit(ulam::Ref<ulam::ast::EmptyStmt> node) override;

    bool in_main() const { return _stack.size() == 1; }
    bool codegen_enabled() const;

    const std::string& data() const { return _data; }

    ulam::sema::ExprRes funcall(
        ulam::Ref<ulam::Fun> fun,
        ulam::LValue self,
        ulam::sema::ExprResList&& args) override;

protected:
    ulam::Ptr<ulam::sema::Resolver>
    _resolver(bool in_expr, ulam::sema::eval_flags_t flags) override;

    ulam::Ptr<ulam::sema::EvalExprVisitor> _expr_visitor(
        ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) override;

    ulam::Ptr<ulam::sema::EvalInit> _init_helper(
        ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) override;

    ulam::Ptr<ulam::sema::EvalCast> _cast_helper(
        ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) override;

    ulam::Ptr<ulam::sema::EvalFuncall> _funcall_helper(
        ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) override;

    ulam::Ref<ulam::AliasType>
    type_def(ulam::Ref<ulam::ast::TypeDef> node) override;

    ulam::Ptr<ulam::Var> make_var(
        ulam::Ref<ulam::ast::TypeName> type_name,
        ulam::Ref<ulam::ast::VarDef> node,
        bool is_const) override;

    void var_init_expr(
        ulam::Ref<ulam::Var> var,
        ulam::sema::ExprRes&& init,
        bool in_expr) override;
    void var_init_default(ulam::Ref<ulam::Var> var, bool in_expr) override;
    void var_init(ulam::Ref<ulam::Var> var, bool in_expr) override;

    ulam::Ptr<ulam::Var>
    make_which_tmp_var(ulam::Ref<ulam::ast::Which> node) override;

    ulam::sema::ExprRes eval_which_match(
        ulam::Ref<ulam::ast::Expr> expr,
        ulam::Ref<ulam::ast::Expr> case_expr,
        ulam::sema::ExprRes&& expr_res,
        ulam::sema::ExprRes&& case_res) override;

    ulam::sema::ExprRes _eval_expr(
        ulam::Ref<ulam::ast::Expr> expr,
        ulam::sema::eval_flags_t flags_) override;

    ulam::sema::ExprRes _to_boolean(
        ulam::Ref<ulam::ast::Expr> expr,
        ulam::sema::ExprRes&& res,
        ulam::sema::eval_flags_t flags_) override;

private:
    void add_as_cond(
        ulam::Ref<ulam::ast::UnaryOp> as_cond, ulam::Ref<ulam::Type> type);

    unsigned next_tmp_idx() { return ++_tmp_idx; }

    void block_open(bool nospace = false);
    void block_close(bool nospace = false);
    void append(std::string data, bool nospace = false);

    void maybe_wrap_stmt(ulam::Ref<ulam::ast::Stmt> stmt, bool wrap);

    void set_next_prefix(std::string prefix);
    std::string move_next_prefix();

    Stringifier make_stringifier();

    std::string _data;
    std::string _next_prefix;
    unsigned _tmp_idx{0};
    EvalContextStack _ctx_stack;
};

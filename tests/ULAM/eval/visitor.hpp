#pragma once
#include "./stringifier.hpp"
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
        ulam::sema::EvalVisitor{program, flags}, _stringifier{program} {}

    ulam::sema::ExprRes eval(ulam::Ref<ulam::ast::Block> block) override;

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::If> node) override;
    void visit(ulam::Ref<ulam::ast::IfAs> node) override;
    void visit(ulam::Ref<ulam::ast::For> node) override;
    void visit(ulam::Ref<ulam::ast::While> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
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

    void var_init_expr(
        ulam::Ref<ulam::Var> var, ulam::sema::ExprRes&& init) override;
    void var_init_default(ulam::Ref<ulam::Var> var) override;
    void var_init(ulam::Ref<ulam::Var> var) override;

    ulam::sema::ExprRes _eval_expr(
        ulam::Ref<ulam::ast::Expr> expr, ulam::sema::eval_flags_t) override;

private:
    unsigned next_loop_idx() { return ++_loop_idx; }

    void block_open(bool nospace = false);
    void block_close(bool nospace = false);
    void append(std::string data, bool nospace = false);

    void set_next_prefix(std::string prefix);
    std::string move_next_prefix();

    Stringifier _stringifier;
    std::string _data;
    std::string _next_prefix;
    unsigned _loop_idx{0};
};

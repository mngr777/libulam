#pragma once
#include "./flags.hpp"
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
    explicit EvalVisitor(
        ulam::Ref<ulam::Program> program,
        ulam::sema::eval_flags_t flags = ulam::sema::evl::NoFlags):
        ulam::sema::EvalVisitor{program, flags}, _stringifier{program} {}

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::While> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
    void visit(ulam::Ref<ulam::ast::ExprStmt> node) override;

    bool in_main() const { return _stack.size() == 1; }

    bool codegen_enabled() const {
        return in_main() && !(flags() & evl::NoCodegen);
    }

    const std::string& data() const { return _data; }

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

    ulam::Ref<ulam::Var> var_def(
        ulam::Ref<ulam::ast::TypeName> type_name,
        ulam::Ref<ulam::ast::VarDef> node) override;

    ulam::Ptr<ulam::Var> make_var(
        ulam::Ref<ulam::ast::TypeName> type_name,
        ulam::Ref<ulam::ast::VarDef> node) override;

    void
    var_set_init(ulam::Ref<ulam::Var> var, ulam::sema::ExprRes&& init) override;

    ulam::sema::ExprRes _eval_expr(
        ulam::Ref<ulam::ast::Expr> expr, ulam::sema::eval_flags_t) override;

private:
    void append(std::string data, bool nospace = false);

    Stringifier _stringifier;
    std::string _data;
};

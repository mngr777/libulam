#pragma once
#include "./stringifier.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>

class EvalVisitor : public ulam::sema::EvalVisitor {
public:
    EvalVisitor(ulam::Ref<ulam::Program> program):
        ulam::sema::EvalVisitor{program}, _stringifier{program->text_pool()} {}

protected:
    ulam::sema::ExprRes eval_expr(ulam::Ref<ulam::ast::Expr> expr) override;

    ulam::Ptr<ulam::sema::EvalExprVisitor>
    expr_visitor(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalInit>
    init_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalCast>
    cast_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalFuncall>
    funcall_helper(ulam::Ref<ulam::Scope> scope) override;

private:
    Stringifier _stringifier;
};

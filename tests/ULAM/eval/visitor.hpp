#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>

class EvalVisitor : public ulam::sema::EvalVisitor {
public:
    using ulam::sema::EvalVisitor::EvalVisitor;

protected:
    ulam::Ptr<ulam::sema::EvalExprVisitor>
    expr_visitor(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalInit>
    init_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalCast>
    cast_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalFuncall>
    funcall_helper(ulam::Ref<ulam::Scope> scope) override;
};

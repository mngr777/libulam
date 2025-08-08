#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam::sema {

class EvalHelperBase : public EvalBase {
public:
    EvalHelperBase(EvalVisitor& eval, Ref<Program> program, eval_flags_t flags):
        EvalBase{program, flags}, _eval{eval} {}

protected:
    EvalVisitor& eval() { return _eval; }

    ExprRes to_boolean(
        Scope* scope, Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags);

private:
    EvalVisitor& _eval;
};

class EvalHelper : public EvalHelperBase {
public:
    EvalHelper(
        EvalVisitor& eval,
        Ref<Program> program,
        ScopeStack& scope_stack,
        eval_flags_t flags):
        EvalHelperBase{eval, program, flags}, _scope_stack{scope_stack} {}

protected:
    ScopeStack& scope_stack() { return _scope_stack; }
    Scope* scope() { return _scope_stack.top(); }

private:
    ScopeStack& _scope_stack;
};

class ScopedEvalHelper : public EvalHelperBase {
public:
    ScopedEvalHelper(
        EvalVisitor& eval,
        Ref<Program> program,
        Ref<Scope> scope,
        eval_flags_t flags):
        EvalHelperBase{eval, program, flags}, _scope{scope} {}

protected:
    using EvalHelperBase::to_boolean;

    ExprRes to_boolean(Ref<ast::Expr> expr, ExprRes&& res, eval_flags_t flags);

    Ref<Scope> scope() { return _scope; }

private:
    Ref<Scope> _scope;
};

} // namespace ulam::sema

#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/stack.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalHelperBase : public EvalBase {
public:
    EvalHelperBase(EvalVisitor& eval, Ref<Program> program):
        EvalBase{program}, _eval_ref{eval} {}

protected:
    EvalVisitor& eval_ref() { return _eval_ref; }

    // convenience
    eval_flags_t has_flag(eval_flags_t flag) const;
    eval_flags_t flags() const;

private:
    EvalVisitor& _eval_ref;
};

class EvalHelper : public EvalHelperBase {
public:
    EvalHelper(
        EvalVisitor& eval, Ref<Program> program, ScopeStack& scope_stack):
        EvalHelperBase{eval, program}, _scope_stack{scope_stack} {}

protected:
    EvalVisitor* eval() { return &eval_ref(); }
    ScopeStack& scope_stack() { return _scope_stack; }

    Scope* scope();

private:
    ScopeStack& _scope_stack;
};

class ScopedEvalHelper : public EvalHelperBase {
public:
    class EvalVisitorProxy {
        friend ScopedEvalHelper;

    private:
        EvalVisitorProxy(EvalVisitor& eval, Scope* scope);

        EvalVisitorProxy(EvalVisitorProxy&&) = default;
        EvalVisitorProxy& operator=(EvalVisitorProxy&&) = delete;

    public:
        EvalVisitor* operator->() { return &_eval; };
        const EvalVisitor* operator->() const { return &_eval; };

    private:
        EvalVisitor& _eval;
        ScopeRaii _scope_raii;
    };

    ScopedEvalHelper(EvalVisitor& eval, Ref<Program> program, Ref<Scope> scope):
        EvalHelperBase{eval, program}, _scope{scope} {
        assert(scope);
    }

protected:
    EvalVisitorProxy eval() { return {eval_ref(), _scope}; }

    Scope* scope() { return _scope; }

private:
    Scope* _scope;
};

} // namespace ulam::sema

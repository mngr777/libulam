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
    EvalHelperBase(EvalVisitor& eval, Ref<Program> program):
        EvalBase{program}, _eval_ref{eval} {}

protected:
    EvalVisitor& eval_ref() { return _eval_ref; }

    // convenience
    eval_flags_t has_flag(eval_flags_t flag) const { return flags() & flag; }
    eval_flags_t flags() const { return _eval_ref.flags(); }

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

    Scope* scope() { return eval()->scope(); }

private:
    ScopeStack& _scope_stack;
};

class ScopedEvalHelper : public EvalHelperBase {
public:
    class EvalVisitorProxy {
        friend ScopedEvalHelper;

    private:
        EvalVisitorProxy(EvalVisitor& eval, Scope* scope):
            _eval{eval}, _scope_raii(_eval.scope_raii(scope)) {}

        EvalVisitorProxy(EvalVisitorProxy&&) = default;
        EvalVisitorProxy& operator=(EvalVisitorProxy&&) = delete;

    public:
        EvalVisitor* operator->() { return &_eval; };
        const EvalVisitor* operator->() const { return &_eval; };

    private:
        EvalVisitor& _eval;
        EvalVisitor::ScopeRaii _scope_raii;
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

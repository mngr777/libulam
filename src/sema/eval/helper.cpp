#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

// EvalHelperBase

eval_flags_t EvalHelperBase::has_flag(eval_flags_t flag) const {
    return flags() & flag;
}

eval_flags_t EvalHelperBase::flags() const { return _eval_ref.flags(); }

// EvalHelper

Scope* EvalHelper::scope() { return eval()->scope(); }

// ScopedEvalHelper

ScopedEvalHelper::EvalVisitorProxy::EvalVisitorProxy(
    EvalVisitor& eval, Scope* scope):
    _eval{eval}, _scope_raii(_eval.scope_raii(scope)) {}

} // namespace ulam::sema

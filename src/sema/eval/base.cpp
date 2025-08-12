#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

// EvalBase::FlagsRaii

EvalBase::FlagsRaii::FlagsRaii(EvalVisitor& eval, eval_flags_t flags):
    _eval{&eval}, _old_flags{eval._flags} {
    _eval->_flags = flags;
}

EvalBase::FlagsRaii::FlagsRaii(): _eval{}, _old_flags{evl::NoFlags} {}

EvalBase::FlagsRaii::~FlagsRaii() {
    if (_eval)
        _eval->_flags = _old_flags;
}

EvalBase::FlagsRaii::FlagsRaii(FlagsRaii&& other): FlagsRaii{} {
    operator=(std::move(other));
}

EvalBase::FlagsRaii& EvalBase::FlagsRaii::operator=(FlagsRaii&& other) {
    std::swap(_eval, other._eval);
    std::swap(_old_flags, other._old_flags);
    return *this;
}

// EvalBase::ScopeRaii

EvalBase::ScopeRaii::ScopeRaii(EvalVisitor& eval, Scope* scope):
    _eval{&eval}, _old_scope{eval._scope} {
    _eval->_scope = scope;
}

EvalBase::ScopeRaii::ScopeRaii(): _eval{}, _old_scope{} {}

EvalBase::ScopeRaii::~ScopeRaii() {
    if (_eval)
        _eval->_scope = _old_scope;
}

EvalBase::ScopeRaii::ScopeRaii(ScopeRaii&& other) {
    operator=(std::move(other));
}

EvalBase::ScopeRaii& EvalBase::ScopeRaii::operator=(ScopeRaii&& other) {
    std::swap(_eval, other._eval);
    std::swap(_old_scope, other._old_scope);
    return *this;
}

// EvalBase

bool EvalBase::is_true(const ExprRes& res, bool default_value) {
    assert(res.type()->is(BoolId));
    bool is_truth = default_value;
    res.value().with_rvalue([&](const auto& rval) {
        is_truth = builtins().bool_type()->is_true(rval);
    });
    return is_truth;
}

} // namespace ulam::sema

#include <cassert>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/context.hpp>

namespace ulam {

bool ScopeContextProxy::has_self() const {
    return _ctx.accept(
        [&](const BasicScopeContext* ctx) { return !ctx->self.empty(); },
        [&](const PersScopeContext*) { return false; });
}

LValue ScopeContextProxy::self() const {
    return _ctx.accept(
        [&](const BasicScopeContext* ctx) {
            return (!ctx->self.empty() || !_parent) ? ctx->self
                                                    : _parent->ctx().self();
        },
        [&](const PersScopeContext*) { return LValue{}; });
}

void ScopeContextProxy::set_self(LValue lval, Ref<Class> eff_cls) {
    _ctx.accept(
        [&](BasicScopeContext* ctx) {
            lval.set_is_xvalue(false);
            ctx->self = std::move(lval);
            ctx->eff_cls = eff_cls;
        },
        [&](PersScopeContext*) { assert(false); });
}

Ref<Class> ScopeContextProxy::self_cls() const {
    return _ctx.accept([&](const auto ctx) {
        return (ctx->self_cls || !_parent) ? ctx->self_cls
                                           : _parent->ctx().self_cls();
    });
}

void ScopeContextProxy::set_self_cls(Ref<Class> cls) {
    _ctx.accept([&](auto* ctx) { ctx->self_cls = cls; });
}

Ref<Class> ScopeContextProxy::eff_cls() const {
    return _ctx.accept(
        [&](const BasicScopeContext* ctx) {
            if (!ctx->self.empty() || !_parent)
                return ctx->eff_cls ? ctx->eff_cls : self_cls();
            return _parent->ctx().eff_cls();
        },
        [&](const PersScopeContext*) { return self_cls(); });
}

void ScopeContextProxy::set_eff_cls(Ref<Class> cls) {
    _ctx.accept(
        [&](BasicScopeContext* ctx) { ctx->eff_cls = cls; },
        [&](PersScopeContext*) { assert(false); });
}

} // namespace ulam

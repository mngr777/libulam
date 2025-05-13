#pragma once
#include "libulam/detail/variant.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Scope;

struct BasicScopeContext {
    LValue self;
    Ref<Class> self_cls{}; // Self
    Ref<Class> eff_cls{};  // effective self class for `if (self as Class) {}`
};

struct PersScopeContext {
    Ref<Class> self_cls{}; // Self
};

class ScopeContextProxy {
public:
    ScopeContextProxy(BasicScopeContext& ctx, Scope* parent):
        _ctx{&ctx}, _parent{parent} {}

    ScopeContextProxy(PersScopeContext& ctx, Scope* parent):
        _ctx{&ctx}, _parent{parent} {}

    ScopeContextProxy() {}

    bool has_self() const;

    LValue self() const;
    void set_self(LValue lval, Ref<Class> eff_cls = {});

    Ref<Class> self_cls() const;
    void set_self_cls(Ref<Class> cls);

    Ref<Class> eff_cls() const;
    void set_eff_cls(Ref<Class> cls);

private:
    using Variant = detail::Variant<BasicScopeContext*, PersScopeContext*>;

    Variant _ctx;
    Scope* _parent{};
};

} // namespace ulam

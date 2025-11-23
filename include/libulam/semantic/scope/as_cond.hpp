#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>

namespace ulam {

class Class;
class LValue;

class AsCondScope : public BasicScope {
public:
    AsCondScope(
        Ref<Class> self_cls,
        LValue self,
        Scope* parent,
        scope_flags_t flags = scp::NoFlags):
        BasicScope{parent, (scope_flags_t)(flags | scp::AsCond | scp::Self)},
        _self_cls{self_cls},
        _self{self} {}

    AsCondScope(
        Ref<Var> var, Scope* parent, scope_flags_t flags = scp::NoFlags):
        BasicScope{parent, (scope_flags_t)(flags | scp::AsCond)},
        _self_cls{},
        _self{} {
        set(var->name_id(), var);
    }

    Ref<Class> eff_cls() const override {
        return _self_cls ? _self_cls : Scope::eff_cls();
    }

    bool has_self() const override { return _self_cls; }

    LValue self() const override { return has_self() ? _self : Scope::self(); }

private:
    Ref<Class> _self_cls;
    LValue _self;
};

} // namespace ulam

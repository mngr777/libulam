#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>

namespace ulam {

class Class;

class ClassScope : public PersScope {
public:
    ClassScope(
        Ref<Class> cls, Scope* parent, scope_flags_t flags = scp::NoFlags):
        PersScope{parent, (scope_flags_t)(flags | scp::Class)}, _cls{cls} {}

    Ref<Class> self_cls() const override { return _cls; }
    Ref<Class> eff_cls() const override { return _cls; }

private:
    Ref<Class> _cls{};
};

} // namespace ulam

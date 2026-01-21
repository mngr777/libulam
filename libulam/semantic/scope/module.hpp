#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>

namespace ulam {

class Module;

class ModuleScope : public PersScope {
public:
    ModuleScope(Ref<Module> module, Scope* parent, scope_flags_t flags):
        PersScope{parent, (scope_flags_t)(flags | scp::Module)},
        _module{module} {}

    Ref<Module> module() const override { return _module; }

private:
    Ref<Module> _module{};
};

} // namespace ulam

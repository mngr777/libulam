#pragma once
#include <cassert>
#include <libulam/semantic/scope.hpp>

namespace ulam {

class Module;

class ModuleScope : public PersScope {
public:
    ModuleScope(Ref<Module> module, Scope* parent, scope_flags_t flags):
        PersScope{parent, flags}, _module{module} {}

    Ref<Module> module() override { return _module; }

private:
    Ref<Module> _module;
};

} // namespace ulam

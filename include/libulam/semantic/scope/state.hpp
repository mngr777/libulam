#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/version.hpp>

namespace ulam {

class PersScope;

class PersScopeState {
public:
    PersScopeState(Ref<PersScope> scope, ScopeVersion version):
        _scope{scope}, _version{version} {}
    PersScopeState(): PersScopeState{{}, NoScopeVersion} {}

    operator bool() const { return _scope; }

    Ref<PersScope> scope() { return _scope; }
    ScopeVersion version() { return _version; }

private:
    Ref<PersScope> _scope;
    ScopeVersion _version;
};

} // namespace ulam

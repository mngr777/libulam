#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/version.hpp>

namespace ulam {

class Scope;

class ScopeObject {
public:
    enum ResState { NotResolved, Resolving, Resolved, Unresolvable };

    ResState res_state() const { return _res_state; }

    void set_res_state(ResState state) { _res_state = state; }

    ScopeVersion scope_version() const {
        assert(_scope_version != NoScopeVersion);
        return _scope_version;
    }

    void set_scope_version(ScopeVersion version) {
        assert(version != NoScopeVersion);
        _scope_version = version;
    }

private:
    ResState _res_state{NotResolved};
    ScopeVersion _scope_version{NoScopeVersion};
};

} // namespace ulam

#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/state.hpp>

namespace ulam {

class Scope;

class ScopeObject {
public:
    enum ResState { NotResolved, Resolving, Resolved, Unresolvable };

    ResState res_state() const { return _res_state; }

    void set_res_state(ResState state) { _res_state = state; }

    PersScopeState pers_scope_state() const {
        assert(_pers_scope_state);
        return _pers_scope_state;
    }

    void set_pers_scope_state(PersScopeState state) {
        assert(!_pers_scope_state);
        _pers_scope_state = state;
    }

private:
    ResState _res_state{NotResolved};
    PersScopeState _pers_scope_state;
};

} // namespace ulam

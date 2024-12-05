#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/state.hpp>

namespace ulam {

class Scope;

class ScopeObject {
public:
    PersScopeState pers_scope_state() const { return _pers_scope_state; }

    void set_pers_scope_state(PersScopeState state) {
        assert(!_pers_scope_state);
        _pers_scope_state = state;
    }
private:
    PersScopeState _pers_scope_state;
};

} // namespace ulam

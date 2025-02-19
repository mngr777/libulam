#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/version.hpp>

namespace ulam {

class Class;

class Decl {
public:
    enum State { NotResolved, Resolving, Resolved, Unresolvable };

    bool is_ready() const { return _state == Resolved; }
    bool is_resolving() const { return _state == Resolving; };

    bool has_cls() const;
    Ref<Class> cls();
    Ref<const Class> cls() const;
    void set_cls(Ref<Class> cls);

    State state() const { return _state; }
    void set_state(State state) { _state = state; }

    ScopeVersion scope_version() const;
    void set_scope_version(ScopeVersion version);

private:
    Ref<Class> _cls{};
    State _state{NotResolved};
    ScopeVersion _scope_version{NoScopeVersion};
};

} // namespace ulam

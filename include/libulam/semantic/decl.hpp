#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/version.hpp>

namespace ulam {

class Class;
class Module;

class Decl {
public:
    enum State {
        NotResolved,
        Initializing, // class only
        Initialized, // class only
        Resolving,
        Resolved,
        Unresolvable
    };

    Decl() {}

    bool is_ready() const { return _state == Resolved; }
    bool is_resolving() const { return _state == Resolving; };

    bool state_is(State state) const { return _state == state; }

    bool has_module() const;
    Ref<Module> module() const;
    void set_module(Ref<Module> module);

    bool has_cls() const;
    Ref<Class> cls() const;
    void set_cls(Ref<Class> cls);

    State state() const { return _state; }
    void set_state(State state) { _state = state; }

    ScopeVersion scope_version() const;
    void set_scope_version(ScopeVersion version);

private:
    Ref<Module> _module{};
    Ref<Class> _cls{};
    State _state{NotResolved};
    ScopeVersion _scope_version{NoScopeVersion};
};

} // namespace ulam

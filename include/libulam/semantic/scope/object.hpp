#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/version.hpp>

namespace ulam {

class Class;
class Scope;

// TODO: this should be called something like `Decl'

class ScopeObject {
public:
    enum ResState { NotResolved, Resolving, Resolved, Unresolvable };

    bool has_cls() const { return _cls; }

    Ref<Class> cls() {
        assert(_cls);
        return _cls;
    }
    Ref<const Class> cls() const {
        assert(_cls);
        return _cls;
    }
    void set_cls(Ref<Class> cls) {
        assert(!_cls);
        assert(cls);
        _cls = cls;
    }

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
    Ref<Class> _cls{};
    ResState _res_state{NotResolved};
    ScopeVersion _scope_version{NoScopeVersion};
};

} // namespace ulam

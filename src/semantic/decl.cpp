#include <cassert>
#include <libulam/semantic/decl.hpp>

namespace ulam {

bool Decl::has_cls() const {
    return _cls;
}

Ref<Class> Decl::cls() const {
    assert(_cls);
    return _cls;
}

void Decl::set_cls(Ref<Class> cls) {
    assert(!_cls);
    assert(cls);
    _cls = cls;
}

ScopeVersion Decl::scope_version() const {
    assert(_scope_version != NoScopeVersion);
    return _scope_version;
}

void Decl::set_scope_version(ScopeVersion version) {
    assert(version != NoScopeVersion);
    _scope_version = version;
}

} // namespace ulam

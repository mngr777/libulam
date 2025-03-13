#include <cassert>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

bool Decl::has_module() const { return _module; }

Ref<Module> Decl::module() const {
    assert(_module);
    return _module;
}

void Decl::set_module(Ref<Module> module) {
    assert(!_module);
    assert(module);
    _module = module;
}

bool Decl::has_cls() const { return _cls; }

Ref<Class> Decl::cls() const {
    assert(_cls);
    return _cls;
}

void Decl::set_cls(Ref<Class> cls) {
    assert(!_cls);
    assert(cls);
    _cls = cls;
    _module = cls->module();
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

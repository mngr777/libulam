#include "libulam/semantic/scope/version.hpp"
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

bool Decl::has_cls_tpl() const { return _cls_tpl; }

Ref<ClassTpl> Decl::cls_tpl() const {
    assert(_cls_tpl);
    return _cls_tpl;
}

void Decl::set_cls_tpl(Ref<ClassTpl> cls_tpl) {
    assert(!_cls_tpl);
    _cls_tpl = cls_tpl;
}

bool Decl::has_scope_version() const {
    return _scope_version != NoScopeVersion;
}

ScopeVersion Decl::scope_version() const {
    assert(has_scope_version());
    return _scope_version;
}

void Decl::set_scope_version(ScopeVersion version) {
    assert(!has_scope_version());
    _scope_version = version;
}

} // namespace ulam

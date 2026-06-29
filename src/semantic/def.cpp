#include <libulam/assert.hpp>
#include <libulam/semantic/def.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

bool Def::has_module() const { return _module; }

Ref<Module> Def::module() const {
    ulam_assert(_module);
    return _module;
}

void Def::set_module(Ref<Module> module) {
    ulam_assert(!_module);
    ulam_assert(module);
    _module = module;
}

bool Def::has_cls() const { return _cls; }

Ref<Class> Def::cls() const {
    ulam_assert(_cls);
    return _cls;
}

void Def::set_cls(Ref<Class> cls) {
    ulam_assert(!_cls);
    ulam_assert(cls);
    _cls = cls;
    _module = cls->module();
}

bool Def::has_cls_tpl() const { return _cls_tpl; }

Ref<ClassTpl> Def::cls_tpl() const {
    ulam_assert(_cls_tpl);
    return _cls_tpl;
}

void Def::set_cls_tpl(Ref<ClassTpl> cls_tpl) {
    ulam_assert(!_cls_tpl);
    _cls_tpl = cls_tpl;
}

bool Def::has_scope_version() const {
    return _scope_version != NoScopeVersion;
}

scope_version_t Def::scope_version() const {
    ulam_assert(has_scope_version());
    return _scope_version;
}

void Def::set_scope_version(scope_version_t version) {
    ulam_assert(!has_scope_version());
    _scope_version = version;
}

} // namespace ulam

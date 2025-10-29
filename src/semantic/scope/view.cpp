#include "libulam/semantic/module.hpp"
#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/scope/view.hpp>

namespace ulam {

PersScopeView::PersScopeView(PersScope* scope, ScopeVersion version):
    Scope{},
    _scope{scope},
    _version{version != NoScopeVersion ? version : scope->version()} {}

void PersScopeView::sync() { set_version(_scope->version()); }

std::pair<str_id_t, Scope::Symbol*> PersScopeView::advance() {
    if (_version == scope()->version())
        return {NoStrId, nullptr};
    assert(_version < scope()->version());
    ++_version;
    auto name_id = last_change();
    assert(name_id != NoStrId && get(name_id));
    return {name_id, get(name_id)};
}

Scope* PersScopeView::parent(scope_flags_t flags) {
    return scope()->parent(flags);
}

Ref<Module> PersScopeView::module() {
    return scope()->module();
}

scope_flags_t PersScopeView::flags() const { return scope()->flags(); }

Scope::Symbol* PersScopeView::get(str_id_t name_id, bool current) {
    return scope()->get(name_id, _version, current);
}

Scope::Symbol* PersScopeView::get_local(str_id_t name_id) {
    return scope()->get_local(name_id, _version);
}

ScopeContextProxy PersScopeView::ctx() { return scope()->ctx(); }

str_id_t PersScopeView::last_change() const {
    return scope()->last_change(_version);
}

void PersScopeView::set_version(ScopeVersion version) {
    assert(version != NoScopeVersion);
    assert(version <= scope()->version());
    _version = version;
}

void PersScopeView::set_version_after(ScopeVersion version) {
    assert(version != NoScopeVersion);
    set_version(version + 1);
}

ScopeIter PersScopeView::begin() {
    return ScopeIter{PersScopeIter{PersScopeView{_scope, 0}}};
}

ScopeIter PersScopeView::end() { return ScopeIter{PersScopeIter{}}; }

bool PersScopeView::operator==(const PersScopeView& other) const {
    return _scope == other._scope && _version == other._version;
}

bool PersScopeView::operator!=(const PersScopeView& other) const {
    return !operator==(other);
}

Scope::Symbol* PersScopeView::do_set(str_id_t name_id, Symbol&& symbol) {
    assert(_version == scope()->version());
    auto sym = scope()->do_set(name_id, std::move(symbol));
    sync();
    return sym;
}

PersScope* PersScopeView::scope() {
    assert(_scope);
    return _scope;
}

const PersScope* PersScopeView::scope() const {
    assert(_scope);
    return _scope;
}

} // namespace ulam

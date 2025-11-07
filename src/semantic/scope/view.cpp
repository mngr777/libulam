#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/scope/view.hpp>

namespace ulam {

PersScopeView::PersScopeView(PersScope* scope, scope_version_t version):
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
    assert(name_id != NoStrId && find(name_id).first);
    return {name_id, find(name_id).first};
}

Scope* PersScopeView::parent(scope_flags_t flags) {
    return scope()->parent(flags);
}

Ref<Module> PersScopeView::module() const { return scope()->module(); }

Ref<Class> PersScopeView::self_cls() const { return scope()->self_cls(); }

Ref<Class> PersScopeView::eff_cls() const { return scope()->eff_cls(); }

scope_flags_t PersScopeView::flags() const { return scope()->flags(); }

bool PersScopeView::has(str_id_t name_id, const GetParams& params) const {
    return scope()->has(name_id, _version, params);
}

Scope::Symbol* PersScopeView::get(str_id_t name_id, const GetParams& params) {
    return scope()->get(name_id, _version, params);
}

Scope::FindRes PersScopeView::find(str_id_t name_id) {
    return scope()->find(name_id, _version);
}

str_id_t PersScopeView::last_change() const {
    return scope()->last_change(_version);
}

void PersScopeView::set_version(scope_version_t version) {
    assert(version != NoScopeVersion);
    assert(version <= scope()->version());
    _version = version;
}

void PersScopeView::set_version_after(scope_version_t version) {
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

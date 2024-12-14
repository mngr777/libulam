#include <libulam/semantic/scope/view.hpp>

namespace ulam {

PersScopeView::PersScopeView(Ref<PersScope> scope, ScopeVersion version):
    Scope{}, _scope{scope}, _version{version} {}

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

Ref<Scope> PersScopeView::parent() { return scope()->parent(); }

Ref<const Scope> PersScopeView::parent() const { return scope()->parent(); }

void PersScopeView::for_each(ItemCb cb) { scope()->for_each(cb, _version); }

ScopeFlags PersScopeView::flags() const { return scope()->flags(); }

Scope::Symbol* PersScopeView::get(str_id_t name_id, bool current) {
    return scope()->get(name_id, _version, current);
}

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

Scope::Symbol* PersScopeView::do_set(str_id_t name_id, Symbol&& symbol) {
    assert(_version == scope()->version());
    auto sym = scope()->do_set(name_id, std::move(symbol));
    sync();
    return sym;
}

Ref<PersScope> PersScopeView::scope() {
    assert(_scope);
    return _scope;
}

Ref<const PersScope> PersScopeView::scope() const {
    assert(_scope);
    return _scope;
}

} // namespace ulam

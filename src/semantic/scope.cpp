#include "libulam/semantic/scope/version.hpp"
#include <libulam/semantic/scope.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

// BasicScope

Scope::Symbol* BasicScope::get(str_id_t name_id, bool current) {
    auto sym = _symbols.get(name_id);
    return (sym || current || !parent()) ? sym : parent()->get(name_id);
}

void BasicScope::for_each(ItemCb cb) {
    for (auto& pair : _symbols)
        cb(pair.first, pair.second);
}

// PersScopeProxy

PersScopeProxy::PersScopeProxy(Ref<PersScope> scope, ScopeVersion version):
    Scope{{}, scope->flags()}, _scope{scope}, _version{version} {}

void PersScopeProxy::sync() { set_version(_scope->version()); }

std::pair<str_id_t, Scope::Symbol*> PersScopeProxy::advance() {
    if (_version == _scope->version())
        return {NoStrId, nullptr};
    assert(_version < _scope->version());
    ++_version;
    auto name_id = last_change();
    assert(name_id != NoStrId && get(name_id));
    return {name_id, get(name_id)};
}

void PersScopeProxy::for_each(ItemCb cb) { _scope->for_each(cb, _version); }

Scope::Symbol* PersScopeProxy::get(str_id_t name_id, bool current) {
    return _scope->get(name_id, _version, current);
}

str_id_t PersScopeProxy::last_change() const {
    return _scope->last_change(_version);
}

void PersScopeProxy::set_version(ScopeVersion version) {
    assert(version != NoScopeVersion);
    assert(version <= _scope->version());
    _version = version;
}

Scope::Symbol* PersScopeProxy::do_set(str_id_t name_id, Symbol&& symbol) {
    assert(_version == _scope->version());
    auto sym = _scope->do_set(name_id, std::move(symbol));
    sync();
    return sym;
}

// PersScope

void PersScope::for_each(ItemCb cb, ScopeVersion version) {
    for (auto& pair : _symbols) {
        auto& [name_id, vsyms] = pair;
        for (auto& vsym : vsyms) {
            if (vsym.version < version) {
                cb(name_id, vsym.symbol);
                break;
            }
        }
    }
}

Scope::Symbol* PersScope::get(str_id_t name_id, bool current) {
    return get(name_id, _version, current);
}

Scope::Symbol* PersScope::get(str_id_t name_id, Version version, bool current) {
    Scope::Symbol* sym{};
    auto it = _symbols.find(name_id);
    if (it != _symbols.end()) {
        auto& vsyms = it->second;
        for (auto& vsym : vsyms) {
            if (vsym.version < version)
                sym = &vsym.symbol;
            break;
        }
    }
    return (sym || current || !parent()) ? sym : parent()->get(name_id);
}

str_id_t PersScope::last_change(Version version) const {
    assert(version <= _changes.size());
    return (version > 0) ? _changes[version - 1] : NoStrId;
}

Scope::Symbol* PersScope::do_set(str_id_t name_id, Scope::Symbol&& symbol) {
    auto version_ = version();
    auto [it, _] = _symbols.emplace(name_id, SymbolVersionList{});
    SymbolVersionList& vsyms = it->second;
    assert(vsyms.size() == 0 || vsyms.front().version <= _version);
    vsyms.emplace_front(_version++, std::move(symbol));
    _changes.push_back(name_id);
    auto sym = &vsyms.front().symbol;
    sym->as_scope_obj()->set_scope_version(version_);
    return sym;
}

} // namespace ulam

#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/view.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

Ref<const Var> ScopeBase::self() const {
    return (_self || parent()) ? parent()->self() : _self;
}

void ScopeBase::set_self(Ref<Var> self) {
    assert(!_self);
    _self = self;
}

// BasicScope

Scope::Symbol* BasicScope::get(str_id_t name_id, bool current) {
    auto sym = _symbols.get(name_id);
    return (sym || current || !parent()) ? sym : parent()->get(name_id);
}

void BasicScope::for_each(ItemCb cb) {
    for (auto& pair : _symbols)
        cb(pair.first, pair.second);
}

// PersScope

Ptr<PersScopeView> PersScope::view(ScopeVersion version) {
    return make<PersScopeView>(this, version);
}

Ptr<PersScopeView> PersScope::view() {
    return make<PersScopeView>(this, version());
}

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
    sym->as_decl()->set_scope_version(version_);
    return sym;
}

} // namespace ulam

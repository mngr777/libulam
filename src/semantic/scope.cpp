#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/scope/view.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

// Scope

Scope::Symbol* Scope::get_local(str_id_t name_id) {
    if (is(scp::Module))
        return get(name_id);
    auto scope = parent(scp::Module);
    if (scope)
        return scope->get(name_id);
    return {};
}

// ScopeBase

Ref<Scope> ScopeBase::parent(ScopeFlags flags) {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : _parent->parent(flags);
}

Ref<const Scope> ScopeBase::parent(ScopeFlags flags) const {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : _parent->parent(flags);
}

bool ScopeBase::has_self() const { return !_self.lval.empty(); }

// TODO: PersScope doesn't need `self'
LValue ScopeBase::self() {
    return (has_self() || !parent()) ? _self.lval : parent()->self();
}

Ref<Class> ScopeBase::eff_self_cls() {
    auto eff_cls =
        (has_self() || !parent()) ? _self.cls : parent()->eff_self_cls();
    return eff_cls ? eff_cls : self_cls();
}

void ScopeBase::set_self(LValue self, Ref<Class> cls) {
    assert(_self.lval.empty());
    _self = {self, cls};
}

Ref<Class> ScopeBase::self_cls() {
    return (_self_cls || !parent()) ? _self_cls : parent()->self_cls();
}

void ScopeBase::set_self_cls(Ref<Class> cls) {
    assert(!_self_cls);
    _self_cls = cls;
}

// BasicScope

Scope::Symbol* BasicScope::get(str_id_t name_id, bool current) {
    auto sym = _symbols.get(name_id);
    return (sym || current || !parent()) ? sym : parent()->get(name_id);
}

// PersScope

Ptr<PersScopeView> PersScope::view(ScopeVersion version) {
    return make<PersScopeView>(this, version);
}

Ptr<PersScopeView> PersScope::view() {
    return make<PersScopeView>(this, version());
}

PersScopeIterator PersScope::begin() {
    return PersScopeIterator(PersScopeView{this, 0});
}

PersScopeIterator PersScope::end() { return PersScopeIterator{}; }

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
    auto [it, _] = _symbols.emplace(name_id, SymbolVersionList{});
    SymbolVersionList& vsyms = it->second;
    assert(vsyms.size() == 0 || vsyms.front().version <= _version);
    vsyms.emplace_front(_version++, std::move(symbol));
    _changes.push_back(name_id);
    return &vsyms.front().symbol;
}

} // namespace ulam

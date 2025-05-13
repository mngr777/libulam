#include "libulam/semantic/scope/flags.hpp"
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/scope/view.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

// Scope

const Scope* Scope::parent(scope_flags_t flags) const {
    return const_cast<Scope*>(this)->parent(flags);
}

bool Scope::is(scope_flags_t flags_) const { return flags() & flags_; }

bool Scope::in(scope_flags_t flags_) const {
    return is(flags_) || (parent() && parent()->in(flags_));
}

bool Scope::has(str_id_t name_id, bool current) const {
    return get(name_id, current);
}

const Scope::Symbol* Scope::get(str_id_t name_id, bool current) const {
    return const_cast<Scope*>(this)->get(name_id, current);
}

Scope::Symbol* Scope::get_local(str_id_t name_id) {
    // ??
    if (is(scp::Class)) {
        assert(parent());
        return parent()->get(name_id);
    }
    auto sym = get(name_id, true);
    return (sym || !parent()) ? sym : parent()->get_local(name_id);
}

const ScopeContextProxy Scope::ctx() const {
    return const_cast<Scope*>(this)->ctx();
}

// ScopeBase

Scope* ScopeBase::parent(scope_flags_t flags) {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : nullptr;
}

// BasicScope

BasicScope::BasicScope(Scope* parent, scope_flags_t flags):
    ScopeBase{parent, flags} {
    assert(!is(scp::Persistent));
}

Scope::Symbol* BasicScope::get(str_id_t name_id, bool current) {
    return current ? _symbols.get(name_id) : do_get(name_id, ctx().eff_cls());
}

ScopeContextProxy BasicScope::ctx() { return {_ctx, parent()}; }

Scope::Symbol* BasicScope::do_get(str_id_t name_id, Ref<Class> eff_cls) {
    Scope* next = this;
    while (next) {
        auto sym = next->get(name_id, true);
        if (sym)
            return sym;
        next = next->parent();
        if (next && next->is(scp::Class) && next->ctx().self_cls() != eff_cls)
            next = eff_cls->scope();
    }
    return {};
}

Scope::Symbol* BasicScope::do_set(str_id_t name_id, Symbol&& symbol) {
    return _symbols.set(name_id, std::move(symbol));
}

// PersScope

PersScopeView PersScope::view(ScopeVersion version) {
    return PersScopeView{this, version};
}

PersScopeView PersScope::view() { return PersScopeView{this, version()}; }

PersScopeIterator PersScope::begin() {
    return PersScopeIterator(PersScopeView{this, 0});
}

PersScopeIterator PersScope::end() { return PersScopeIterator{}; }

Scope::Symbol* PersScope::get(str_id_t name_id, bool current) {
    return get(name_id, _version, current);
}

ScopeContextProxy PersScope::ctx() { return ScopeContextProxy{_ctx, parent()}; }

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

bool PersScope::has(str_id_t name_id, bool current) const {
    return has(name_id, version(), current);
}

bool PersScope::has(str_id_t name_id, Version version, bool current) const {
    return get(name_id, version, current);
}

const Scope::Symbol*
PersScope::get(str_id_t name_id, Version version, bool current) const {
    return const_cast<PersScope*>(this)->get(name_id, version, current);
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

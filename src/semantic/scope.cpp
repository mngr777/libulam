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

const ScopeContextProxy Scope::ctx() const {
    return const_cast<Scope*>(this)->ctx();
}

// ScopeBase

Scope* ScopeBase::parent(scope_flags_t flags) {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : _parent->parent(flags);
}

// BasicScope

BasicScope::BasicScope(Scope* parent, scope_flags_t flags):
    ScopeBase{parent, flags} {
    assert(!is(scp::Persistent));
}

Scope::Symbol* BasicScope::get(str_id_t name_id, bool current) {
    return current ? _symbols.get(name_id)
                   : do_get(name_id, ctx().eff_cls(), false);
}

Scope::Symbol* BasicScope::get_local(str_id_t name_id) {
    return do_get(name_id, ctx().eff_cls(), true);
}

ScopeContextProxy BasicScope::ctx() { return {_ctx, parent()}; }

Scope::Symbol*
BasicScope::do_get(str_id_t name_id, Ref<Class> eff_cls, bool local) {
    // * if effective Self class doesn't match parent class scope Self (i.e.
    // `self as Type` is used):
    //   - seacrh current scope up to class scope;
    //   - store current module scope;
    //   - continue search in effective Self class scope up to module scope;
    //   - switch to stored module scope and continue.
    // * if searching for `local` symbol:
    //   - search current scope up to class scope;
    //   - switch to module scope (skipping class scopes) and continue.
    Scope* scope = this;
    Scope* module_scope{};
    while (true) {
        auto sym = scope->get(name_id, true);
        if (sym)
            return sym;
        scope = scope->parent();
        if (!scope)
            return {};
        if (scope->is(scp::Class) &&
            (local || scope->ctx().self_cls() != eff_cls)) {
            // store current module scope
            module_scope = scope->parent(scp::Module);
            assert(module_scope);
            if (local) {
                // skip class scopes
                scope = module_scope;
            } else {
                // go to effective Self scope
                scope = eff_cls->scope();
            }
        } else if (scope->is(scp::Module) && module_scope) {
            // go back to current module scope
            scope = module_scope;
        }
    }
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

bool PersScope::has(str_id_t name_id, bool current) const {
    return has(name_id, version(), current);
}

bool PersScope::has(str_id_t name_id, Version version, bool current) const {
    return get(name_id, version, current);
}

Scope::Symbol* PersScope::get(str_id_t name_id, bool current) {
    return get(name_id, _version, current);
}

Scope::Symbol* PersScope::get_local(str_id_t name_id) {
    return get_local(name_id, version());
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

Scope::Symbol* PersScope::get_local(str_id_t name_id, Version version) {
    // NOTE: global types in module environment scope (scp::ModuleEnv, parent of
    // scp::Module) count as local symbols: t3875
    assert(in(scp::Module));
    return (is(scp::Module)) ? get(name_id, version)
                             : parent(scp::Module)->get(name_id);
}

const Scope::Symbol*
PersScope::get_local(str_id_t name_id, Version version) const {
    return const_cast<PersScope*>(this)->get_local(name_id, version);
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

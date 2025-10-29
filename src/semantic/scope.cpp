#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/scope/view.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

// Scope

Scope::Scope() {}

Scope::~Scope() {}

const Scope* Scope::parent(scope_flags_t flags) const {
    return const_cast<Scope*>(this)->parent(flags);
}

Ref<Module> Scope::module() {
    auto module_scope = parent(scp::Module);
    return module_scope ? module_scope->module() : nullptr;
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

ScopeIter BasicScope::begin() { return ScopeIter{BasicScopeIter{*this}}; }

ScopeIter BasicScope::end() { return ScopeIter{BasicScopeIter{}}; }

// ScopeBase

Scope* ScopeBase::parent(scope_flags_t flags) {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : _parent->parent(flags);
}

Scope::Symbol* ScopeBase::get(str_id_t name_id, bool current) {
    return current ? do_get_current(name_id)
                   : do_get(name_id, ctx().eff_cls(), false);
}

Scope::Symbol* ScopeBase::get_local(str_id_t name_id) {
    return do_get(name_id, ctx().eff_cls(), true);
}

Scope::Symbol* ScopeBase::do_get_current(str_id_t name_id) {
    return _symbols.get(name_id);
}

Scope::Symbol*
ScopeBase::do_get(str_id_t name_id, Ref<Class> eff_cls, bool local) {
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

Scope::Symbol* ScopeBase::do_set(str_id_t name_id, Symbol&& symbol) {
    return _symbols.set(name_id, std::move(symbol));
}

// BasicScope

BasicScope::BasicScope(Scope* parent, scope_flags_t flags):
    ScopeBase{parent, flags} {
    assert(!is(scp::Persistent));
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

// PersScope

PersScopeView PersScope::view(ScopeVersion version) {
    return PersScopeView{this, version};
}

PersScopeView PersScope::view() { return PersScopeView{this, version()}; }

ScopeIter PersScope::begin() {
    return ScopeIter{PersScopeIter{PersScopeView{this, 0}}};
}

ScopeIter PersScope::end() { return ScopeIter{PersScopeIter{}}; }

bool PersScope::has(str_id_t name_id, bool current) const {
    return has(name_id, version(), current);
}

bool PersScope::has(str_id_t name_id, Version version, bool current) const {
    return get(name_id, version, current);
}

Scope::Symbol* PersScope::get(str_id_t name_id, bool current) {
    return get(name_id, version(), current);
}

Scope::Symbol* PersScope::get_local(str_id_t name_id) {
    return get_local(name_id, version());
}

ScopeContextProxy PersScope::ctx() { return ScopeContextProxy{_ctx, parent()}; }

str_id_t PersScope::last_change(Version version) const {
    assert(version <= _changes.size());
    return (version > 0) ? _changes[version - 1] : NoStrId;
}

Scope::Symbol* PersScope::get(str_id_t name_id, Version version, bool current) {
    auto sym = do_get_current(name_id);
    if (sym && sym->as_decl()->scope_version() < version)
        return sym;
    if (current || !parent())
        return nullptr;
    return parent()->get(name_id);
}

const Scope::Symbol*
PersScope::get(str_id_t name_id, Version version, bool current) const {
    return const_cast<PersScope*>(this)->get(name_id, version, current);
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

ScopeVersion PersScope::version() const { return _changes.size(); }

Scope::Symbol* PersScope::do_set(str_id_t name_id, Scope::Symbol&& symbol) {
    auto sym = ScopeBase::do_set(name_id, std::move(symbol));
    auto decl = sym->as_decl();
    assert(!decl->has_scope_version());
    decl->set_scope_version(version());
    _changes.push_back(name_id);
    return sym;
}

} // namespace ulam

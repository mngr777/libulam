#include "libulam/semantic/scope/flags.hpp"
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/class.hpp>
#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/class.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Scope] "
#include "src/debug.hpp"

namespace ulam {

namespace {

bool is_excluded(const Scope::Symbol& sym, const Scope::GetParams& params) {
    return sym.as_decl() == params.except;
}

} // namespace

// Scope

Scope::Scope() {}

Scope::~Scope() {}

const Scope* Scope::parent(scope_flags_t flags) const {
    return const_cast<Scope*>(this)->parent(flags);
}

Ref<Program> Scope::program() const {
    auto scope = parent(scp::Program);
    return scope ? scope->program() : nullptr;
}

Ref<Module> Scope::module() const {
    auto scope = parent(scp::Module);
    return scope ? scope->module() : nullptr;
}

Ref<Class> Scope::self_cls() const {
    auto scope = parent(scp::Class);
    return scope ? scope->self_cls() : nullptr;
}

Ref<Class> Scope::eff_cls() const {
    auto cur = this;
    while ((cur = cur->parent(scp::Class | scp::AsCond))) {
        auto cls = cur->eff_cls();
        if (cls)
            return cls;
    }
    return {};
}

Ref<Fun> Scope::fun() const {
    auto scope = parent(scp::Fun);
    return scope ? scope->fun() : nullptr;
}

const ScopeOptions& Scope::options() const {
    auto program_ = program();
    return program_ ? program_->scope_options() : DefaultScopeOptions;
}

bool Scope::has_self() const { return false; }

LValue Scope::self() const {
    auto cur = this;
    while ((cur = cur->parent(scp::Fun | scp::AsCond))) {
        if (cur->has_self())
            return cur->self();
    }
    return {};
}

bool Scope::is(scope_flags_t flags_) const { return flags() & flags_; }

bool Scope::in(scope_flags_t flags_) const {
    return is(flags_) || (parent() && parent()->in(flags_));
}

bool Scope::has(str_id_t name_id, const GetParams& params) const {
    return get(name_id, params);
}

const Scope::Symbol*
Scope::get(str_id_t name_id, const GetParams& params) const {
    return const_cast<Scope*>(this)->get(name_id, params);
}

bool Scope::defines(str_id_t name_id) const {
    return const_cast<Scope*>(this)->find(name_id).first;
}

ScopeIter BasicScope::begin() { return ScopeIter{BasicScopeIter{*this}}; }

ScopeIter BasicScope::end() { return ScopeIter{BasicScopeIter{}}; }

// ScopeBase

Scope* ScopeBase::parent(scope_flags_t flags) {
    return (!_parent || (flags == scp::NoFlags) || _parent->is(flags))
               ? _parent
               : _parent->parent(flags);
}

Scope::Symbol* ScopeBase::do_set(str_id_t name_id, Symbol&& symbol) {
    return _symbols.set(name_id, std::move(symbol));
}

// BasicScope

BasicScope::BasicScope(Scope* parent, scope_flags_t flags):
    ScopeBase{parent, flags} {
    assert(!is(scp::Persistent));
}

Scope::Symbol* BasicScope::get(str_id_t name_id, const GetParams& params) {
    if (params.current) {
        auto sym = _symbols.get(name_id);
        return sym && !is_excluded(*sym, params) ? sym : nullptr;
    }

    // * if effective Self class doesn't match parent class scope Self (i.e.
    // `self as Type` is used):
    //   - seacrh current scope up to class scope;
    //   - store current module scope;
    //   - continue search in effective Self class scope up to module scope;
    //   - switch to stored module scope and continue.
    // * if searching for `local` symbol:
    //   - search current scope up to class scope;
    //   - switch to module scope (skipping class scopes) and continue.
    Ref<Class> eff_cls_ = eff_cls();
    Scope* scope = this;
    Scope* module_scope{};
    Symbol* fallback{};
    bool use_fallback = options().allow_access_before_def;
    while (true) {
        auto [sym, is_final] = scope->find(name_id);
        if (sym && is_excluded(*sym, params))
            sym = nullptr;

        if (sym && is_final)
            return sym;

        if (!fallback)
            fallback = sym;

        scope = scope->parent();
        if (!scope)
            return use_fallback ? fallback : nullptr;

        if (scope->is(scp::Class | scp::Params) &&
            (params.local || scope->self_cls() != eff_cls_)) {
            // store current module scope
            module_scope = scope->parent(scp::Module);
            assert(module_scope);
            if (params.local) {
                // skip class scopes
                scope = module_scope;
            } else {
                // go to effective Self scope
                scope = eff_cls_->scope();
            }
        } else if (scope->is(scp::Module) && module_scope) {
            // go back to current module scope
            scope = module_scope;
        }
    }
}

Scope::FindRes BasicScope::find(str_id_t name_id) {
    auto sym = _symbols.get(name_id);
    return {sym, (bool)sym};
}

// PersScope

PersScopeView PersScope::view(version_t version) {
    return PersScopeView{this, version};
}

PersScopeView PersScope::view() { return PersScopeView{this, version()}; }

ScopeIter PersScope::begin() {
    return ScopeIter{PersScopeIter{PersScopeView{this, 0}}};
}

ScopeIter PersScope::end() { return ScopeIter{PersScopeIter{}}; }

bool PersScope::has(str_id_t name_id, const GetParams& params) const {
    return has(name_id, version(), params);
}

bool PersScope::has(
    str_id_t name_id, version_t version, const GetParams& params) const {
    return get(name_id, version, params);
}

Scope::Symbol* PersScope::get(str_id_t name_id, const GetParams& params) {
    return get(name_id, version(), params);
}

Scope::FindRes PersScope::find(str_id_t name_id) {
    return find(name_id, version());
}

str_id_t PersScope::last_change(version_t version) const {
    assert(version <= _changes.size());
    return (version > 0) ? _changes[version - 1] : NoStrId;
}

Scope::Symbol*
PersScope::get(str_id_t name_id, version_t version, const GetParams& params) {
    // NOTE: global types in module environment scope (scp::ModuleEnv, parent of
    // scp::Module) count as local symbols: t3875
    if (params.local && is(scp::Class | scp::Params)) {
        assert(in(scp::Module));
        return parent(scp::Module)->get(name_id, params);
    }

    auto [cur_sym, is_final] = find(name_id, version);
    if (cur_sym && is_excluded(*cur_sym, params))
        cur_sym = nullptr;

    if (cur_sym && !is_final && is(scp::Params) &&
        options().prefer_params_in_param_resolution)
        is_final = true;

    if (cur_sym && is_final)
        return cur_sym;

    auto fallback = options().allow_access_before_def ? cur_sym : nullptr;

    if (params.current || !parent())
        return fallback;

    auto sym = parent()->get(name_id, params);
    return sym ? sym : fallback;
}

const Scope::Symbol* PersScope::get(
    str_id_t name_id, version_t version, const GetParams& params) const {
    return const_cast<PersScope*>(this)->get(name_id, version, params);
}

Scope::FindRes PersScope::find(str_id_t name_id, version_t version) {
    auto sym = _symbols.get(name_id);
    bool is_final = sym && sym->as_decl()->scope_version() < version;
    return {sym, is_final};
}

PersScope::version_t PersScope::version() const { return _changes.size(); }

Scope::Symbol* PersScope::do_set(str_id_t name_id, Scope::Symbol&& symbol) {
    auto sym = ScopeBase::do_set(name_id, std::move(symbol));
    auto decl = sym->as_decl();
    assert(!decl->has_scope_version());
    decl->set_scope_version(version());
    _changes.push_back(name_id);
    return sym;
}

} // namespace ulam

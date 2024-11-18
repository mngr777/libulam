#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/export_import.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam {

Sema::Sema(Diag& diag): _diag{diag} {}

Sema::~Sema() {}

void Sema::analyze(ast::Ref<ast::Root> ast) {
    reset();
    if (!ast->program())
        ast->set_program(make<Program>());
    sema::ExportImport export_import{*this, ast};
    export_import.visit(ast);
}

void Sema::reset() {
    while (!_scopes.empty())
        _scopes.pop();
    enter_scope(Scope::Program);
}

void Sema::enter_scope(Ref<Scope> scope) {
    _scopes.emplace(scope, Ptr<Scope>{});
}

void Sema::enter_scope(Scope::Flag flags) {
    auto parent = _scopes.size() ? _scopes.top().first : Ref<Scope>{};
    auto scope = make<Scope>(parent, flags);
    auto scope_ref = ref(scope);
    _scopes.emplace(scope_ref, std::move(scope));
}

void Sema::exit_scope() {
    assert(_scopes.size() > 1);
    _scopes.pop();
}

Ref<Scope> Sema::scope() {
    assert(!_scopes.empty());
    return _scopes.top().first;
}

} // namespace ulam

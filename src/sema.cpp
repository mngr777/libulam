#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/type_init.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam {

Sema::Sema(Diag& diag): _diag{diag}, _program{make<Program>()} {}

void Sema::analyze(ast::Ref<ast::Root> ast) {
    reset();
    sema::TypeInit type_init{*this, ast};
    type_init.visit(ast);
}

Ptr<Program> Sema::move_program() { return std::move(_program); }

void Sema::reset() {
    while (!_scopes.empty())
        _scopes.pop();
    enter_scope(Scope::Program);
}

void Sema::enter_scope(Ref<Scope> scope) {
    _scopes.emplace(scope, Ptr<Scope>{});
}

void Sema::enter_scope(Scope::Flag flags) {
    auto scope = make<Scope>(this->scope(), flags);
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

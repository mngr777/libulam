#include <libulam/sema.hpp>
#include <libulam/sema/type_init.hpp>

namespace ulam {

void Sema::analyze(ast::Ref<ast::Root> ast) {
    reset();
    sema::TypeInit type_init{*this};
    ast::traverse(ast, type_init);
}

void Sema::reset() {
    while (!_scopes.empty())
        _scopes.pop();
    _scopes.emplace(nullptr);
}

void Sema::enter_scope() {
    _scopes.emplace(&scope());
}

void Sema::exit_scope() {
    _scopes.pop();
}

Scope& Sema::scope() {
    assert(!_scopes.empty());
    return _scopes.top();
}

} // namespace ulam

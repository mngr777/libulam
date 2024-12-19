#include <libulam/semantic/scope/stack.hpp>

namespace ulam {

ScopeStack::Raii::Raii(ScopeStack& stack, Ptr<Scope>&& scope): _stack{stack} {
    _stack.push(std::move(scope));
    _scope = _stack.top();
}

ScopeStack::Raii::Raii(ScopeStack& stack, ScopeFlags flags): _stack{stack} {
    _stack.push(flags);
    _scope = _stack.top();
}

ScopeStack::Raii::~Raii() {
    assert(_scope == _stack.top());
    _stack.pop();
}

Ref<Scope> ScopeStack::top() {
    assert(!empty());
    return ref(_stack.top());
}

ScopeStack::Raii ScopeStack::raii(Ptr<Scope>&& scope) {
    return {*this, std::move(scope)};
}

ScopeStack::Raii ScopeStack::raii(ScopeFlags flags) {
    return {*this, flags};
}

void ScopeStack::push(Ptr<Scope>&& scope) { _stack.push(std::move(scope)); }

void ScopeStack::push(ScopeFlags flags) {
    auto parent = !empty() ? top() : Ref<Scope>{};
    push(make<BasicScope>(parent, flags));
}

void ScopeStack::pop() {
    assert(!empty());
    _stack.pop();
}

} // namespace ulam

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <stack>

namespace ulam {

class ScopeStack {
public:
    class Raii {
        friend ScopeStack;
    private:
        Raii(ScopeStack& stack, Ptr<Scope>&& scope);
        Raii(ScopeStack& stack, scope_flags_t flags);

    public:
        ~Raii();

        Raii(Raii&&) = default;
        Raii& operator=(Raii&&) = delete;

    private:
        ScopeStack& _stack;
        Ref<Scope> _scope;
    };

    ScopeStack() {}

    ScopeStack(ScopeStack&&) = default;
    ScopeStack& operator=(ScopeStack&&) = default;

    std::size_t size() const { return _stack.size(); }
    bool empty() const { return _stack.empty(); }

    Ref<Scope> top();

    Raii raii(Ptr<Scope>&& scope);
    Raii raii(scope_flags_t flags);

    void push(Ptr<Scope>&& scope);
    void push(scope_flags_t flags);

    void pop();

private:
    std::stack<Ptr<Scope>> _stack;
};

} // namespace ulam

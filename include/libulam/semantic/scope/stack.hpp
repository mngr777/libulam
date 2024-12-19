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

    public:
        ~Raii();

        Raii(Raii&& other) = default;
        Raii& operator=(Raii&& other) = delete;

    private:
        Raii(ScopeStack& stack, Ptr<Scope>&& scope);
        Raii(ScopeStack& stack, ScopeFlags flags);

        ScopeStack& _stack;
        Ref<Scope> _scope;
    };

    ScopeStack() {}

    ScopeStack(ScopeStack&& other) = default;
    ScopeStack& operator=(ScopeStack&& other) = default;

    bool empty() { return _stack.empty(); }

    Ref<Scope> top();

    Raii raii(Ptr<Scope>&& scope);
    Raii raii(ScopeFlags flags);

    void push(Ptr<Scope>&& scope);
    void push(ScopeFlags flags);

    void pop();

private:
    std::stack<Ptr<Scope>> _stack;
};

} // namespace ulam

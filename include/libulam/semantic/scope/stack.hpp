#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <stack>

namespace ulam {

class ScopeStack {
public:
    bool empty() { return _stack.empty(); }

    Ref<Scope> top() {
        assert(!empty());
        return ref(_stack.top());
    }

    void push(Ptr<Scope>&& scope) { _stack.push(std::move(scope)); }

    void push(ScopeFlags flags) {
        auto parent = !empty() ? top() : Ref<Scope>{};
        push(make<BasicScope>(parent, flags));
    }

    void pop() {
        assert(!empty());
        _stack.pop();
    }

private:
    std::stack<Ptr<Scope>> _stack;
};

} // namespace ulam

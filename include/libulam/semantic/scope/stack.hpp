#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <stack>
#include <type_traits>

namespace ulam {

class ScopeStack {
public:
    using Variant = detail::Variant<BasicScope*, PersScope*, PersScopeView*>;

    template <typename T> class Raii {
        static_assert(std::is_base_of_v<Scope, T>);
        friend ScopeStack;

        Raii(const Raii&) = delete;
        Raii& operator=(const Raii&) = delete;

    private:
        template <typename... Ts>
        Raii(ScopeStack& stack, Ts&&... args):
            _stack{&stack}, _scope{std::forward<Ts>(args)...} {
            stack.push(Variant{get()});
        }

    public:
        Raii(): _stack{}, _scope{nullptr} {}

        ~Raii() {
            if (_stack)
                _stack->pop();
        }

        T* get() { return &_scope; }

    private:
        ScopeStack* _stack;
        T _scope;
    };

    template <typename T, typename... Ts> Raii<T> raii(Ts&&... args) {
        return Raii<T>(*this, std::forward<Ts>(args)...);
    }

    Scope* top() {
        assert(!empty());
        return _stack.top().accept([&](auto scope) -> Scope* { return scope; });
    }

    Variant& top_v() { return _stack.top(); }

    void push(Variant&& scope_v) { _stack.push(std::move(scope_v)); }
    void pop() { _stack.pop(); }

    std::size_t size() const { return _stack.size(); }
    bool empty() const { return _stack.empty(); }

public:
    std::stack<Variant> _stack;
};

} // namespace ulam

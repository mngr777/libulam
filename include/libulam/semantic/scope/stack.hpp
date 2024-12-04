#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <stack>
#include <variant>

namespace ulam {

class ScopeStack {
public:
    bool empty() {
        return _stack.empty();
    }

    template <typename T>
    bool top_is() {
        if constexpr (std::is_same_v<T, TransScope>) {
            return std::holds_alternative<Ptr<TransScope>>(_stack.top());
        } else if constexpr (std::is_same_v<T, PersScopeProxy>) {
            return std::holds_alternative<PersScopeProxy>(_stack.top());
        } else {
            assert(false);
        }
    }

    template <typename T>
    Ref<T> top() {
        if constexpr (std::is_same_v<T, TransScope>) {
            return ref(std::get<Ptr<TransScope>>(_stack.top()));
        } else if constexpr (std::is_same_v<T, PersScopeProxy>) {
            return &std::get<PersScopeProxy>(_stack.top());
        } if constexpr (std::is_same_v<T, Scope>) {
            if (top_is<TransScope>())
                return top<TransScope>();
            return top<PersScopeProxy>();
        } else {
            assert(false);
        }
    }

    void push(Ptr<TransScope>&& scope) { _stack.push(std::move(scope)); }
    void push(PersScopeProxy&& scope) { _stack.push(std::move(scope)); }
    void pop() { _stack.pop(); }

private:
    using TransPtr = Ptr<TransScope>;
    using Variant = std::variant<TransPtr, PersScopeProxy>;

    std::stack<Variant> _stack;
};

} // namespace ulam

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <stack>
#include <variant>

namespace ulam {

// TODO: use ScopeProxy instead of variant
class ScopeStack {
public:
    bool empty() {
        return _stack.empty();
    }

    template <typename T>
    bool top_is() {
        if constexpr (std::is_same_v<T, BasicScope>) {
            return std::holds_alternative<Ptr<BasicScope>>(_stack.top());
        } else if constexpr (std::is_same_v<T, PersScopeProxy>) {
            return std::holds_alternative<PersScopeProxy>(_stack.top());
        } else {
            assert(false);
        }
    }

    template <typename T>
    Ref<T> top() {
        if constexpr (std::is_same_v<T, BasicScope>) {
            return ref(std::get<Ptr<BasicScope>>(_stack.top()));
        } else if constexpr (std::is_same_v<T, PersScopeProxy>) {
            return &std::get<PersScopeProxy>(_stack.top());
        } if constexpr (std::is_same_v<T, Scope>) {
            if (top_is<BasicScope>())
                return top<BasicScope>();
            return top<PersScopeProxy>();
        } else {
            assert(false);
        }
    }

    void push(Ptr<BasicScope>&& scope) { _stack.push(std::move(scope)); }
    void push(PersScopeProxy&& scope) { _stack.push(std::move(scope)); }
    void pop() { _stack.pop(); }

private:
    using BasicPtr = Ptr<BasicScope>;
    using Variant = std::variant<BasicPtr, PersScopeProxy>;

    std::stack<Variant> _stack;
};

} // namespace ulam

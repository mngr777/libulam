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
        if constexpr (std::is_same_v<T, BasicScope>) {
            return std::holds_alternative<Ptr<BasicScope>>(_stack.top());
        } else if constexpr (std::is_same_v<T, PersScopeView>) {
            return std::holds_alternative<PersScopeView>(_stack.top());
        } else {
            assert(false);
        }
    }

    template <typename T>
    Ref<T> top() {
        if constexpr (std::is_same_v<T, BasicScope>) {
            return ref(std::get<Ptr<BasicScope>>(_stack.top()));
        } else if constexpr (std::is_same_v<T, PersScopeView>) {
            return &std::get<PersScopeView>(_stack.top());
        } if constexpr (std::is_same_v<T, Scope>) {
            if (top_is<BasicScope>())
                return top<BasicScope>();
            return top<PersScopeView>();
        } else {
            assert(false);
        }
    }

    void push(Ptr<BasicScope>&& scope) { _stack.push(std::move(scope)); }
    void push(PersScopeView&& scope) { _stack.push(std::move(scope)); }
    void pop() { _stack.pop(); }

private:
    using BasicPtr = Ptr<BasicScope>;
    using Variant = std::variant<BasicPtr, PersScopeView>;

    std::stack<Variant> _stack;
};

} // namespace ulam

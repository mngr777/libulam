#pragma once
#include <any>
#include <cassert>
#include <cstdint>
#include <stack>
#include <utility>

class EvalContextStack {
public:
    using type_t = std::uint16_t;
    static constexpr type_t NoType = 0;

    class Raii {
        friend EvalContextStack;

    private:
        explicit Raii(EvalContextStack& stack): _stack{stack} {}

    public:
        ~Raii() { _stack.pop(); }

        Raii(Raii&& other) = default;
        Raii& operator=(Raii&&) = delete;

    private:
        EvalContextStack& _stack;
    };

    EvalContextStack() {}

    EvalContextStack(const EvalContextStack&) = delete;
    EvalContextStack& operator=(const EvalContextStack&) = delete;

    std::size_t size() const { return _stack.size(); }
    bool empty() const { return _stack.empty(); }

    bool top_type_is(type_t type) const { return top_type() == type; }

    type_t top_type() const {
        return !_stack.empty() ? _stack.top().first : NoType;
    }

    template <typename T> T& top() {
        assert(!empty());
        return std::any_cast<T&>(_stack.top().second);
    }

    template <typename T> void push(T ctx) {
        _stack.emplace(T::Type, std::move(ctx));
    }

    void pop() { _stack.pop(); }

    template <typename T> Raii raii(T ctx) {
        push(ctx);
        return Raii{*this};
    }

private:
    using Pair = std::pair<type_t, std::any>;

    std::stack<Pair> _stack;
};

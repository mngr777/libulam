#pragma once
#include <any>
#include <cassert>
#include <stack>

class EvalContextStack {
public:
    class Raii {
        friend EvalContextStack;

    private:
        template <typename T>
        Raii(EvalContextStack& stack, T&& ctx): _stack{stack} {
            _stack.push<T>(std::move(ctx));
        }

    public:
        ~Raii() {
            _stack.pop();
        }

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

    template <typename T> T& top() {
        assert(!empty());
        return std::any_cast<T&>(_stack.top());
    }

    template <typename T> void push(T ctx) { _stack.push(std::move(ctx)); }
    void pop() { _stack.pop(); }

    template <typename T> Raii raii(T ctx) { return {*this, std::move(ctx)}; }

private:
    std::stack<std::any> _stack;
};

#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>
#include <stack>

namespace ulam::sema {

class EvalStack {
public:
    class Item {
    public:
        Item(Ref<Fun> fun, LValue self) {}

        Ref<Fun> fun() { return _fun; }
        LValue self() { return _self; }

    private:
        Ref<Fun> _fun;
        LValue _self;
    };

    class Raii {
        friend EvalStack;

    private:
        Raii(EvalStack& stack, Ref<Fun> fun, LValue self);

    public:
        ~Raii();

        Raii(Raii&&) = default;
        Raii& operator=(Raii&) = delete;

    private:
        EvalStack& _stack;
    };

    EvalStack() {}

    EvalStack(EvalStack&&) = default;
    EvalStack& operator=(EvalStack&&) = default;

    std::size_t size() const { return _stack.size(); }
    bool empty() const { return _stack.empty(); }

    Item& top() { return _stack.top(); }

    Raii raii(Ref<Fun> fun, LValue self);

    void push(Ref<Fun> fun, LValue self);
    void pop();

private:
    std::stack<Item> _stack;
};

} // namespace ulam::sema

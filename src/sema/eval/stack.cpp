#include <libulam/sema/eval/stack.hpp>
#include <utility>

namespace ulam::sema {

EvalStack::Raii::Raii(EvalStack& stack, Ref<Fun> fun, LValue self):
    _stack{stack} {
    _stack.push(fun, std::move(self));
}

EvalStack::Raii::~Raii() { _stack.pop(); }

EvalStack::Raii EvalStack::raii(Ref<Fun> fun, LValue self) {
    return {*this, fun, std::move(self)};
}

void EvalStack::push(Ref<Fun> fun, LValue self) {
    _stack.emplace(fun, std::move(self));
}

void EvalStack::pop() {
    assert(!empty());
    _stack.pop();
}

} // namespace ulam::sema

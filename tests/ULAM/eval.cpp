#include "./eval.hpp"
#include "./eval/env.hpp"

ulam::sema::ExprRes Eval::do_eval(ulam::Ref<ulam::ast::Block> block) {
    EvalEnv env{_ast->program()};
    auto res = env.eval(block);
    _data = env.gen().code();
    return res;
}

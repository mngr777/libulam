#include "./eval.hpp"
#include "./eval/env.hpp"
#include <libulam/semantic/scope/options.hpp>

ulam::sema::ExprRes Eval::do_eval(ulam::Ref<ulam::ast::Block> block) {
    auto program = _ast->program();
    EvalEnv env{program};

    auto scope_options = ulam::DefaultScopeOptions;
    scope_options.allow_access_before_def = true;
    program->set_scope_options(scope_options);

    auto res = env.eval(block);
    _code = env.gen().code();
    return res;
}

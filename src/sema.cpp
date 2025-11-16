#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

Ref<Program> init(Context& ctx, Ref<ast::Root> ast) {
    sema::Init init{ctx, ast};
    init.analyze();
    return ast->program();
}

bool resolve(Context& ctx, Ref<Program> program) {
    EvalEnv eval_env{program};
    eval_env.resolver(false).resolve(program);
    return true; // TMP
}

} // namespace ulam::sema

#include <libulam/assert.hpp>
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

bool resolve(EvalEnv& env) {
    env.resolver(false).resolve(env.program());
    return true; // TMP
}

bool resolve(Context& ctx, Ref<Program> program) {
    EvalEnv env{program};
    resolve(env);
    return true; // TMP
}

} // namespace ulam::sema

#include <libulam/assert.hpp>
#include <libulam/sema.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

Ref<Program> init(Context& ctx, Ref<ast::Root> ast) {
    ulam_assert(!ast->program());
    ast->set_program(
        make<Program>(ctx, ast->ctx().str_pool(), ast->ctx().text_pool()));
    sema::Init init{ctx, ast};
    init.visit(ast);
    return ast->program();
}

Ref<Module> init(Context& ctx, Ref<ast::Root> ast, Ref<ast::ModuleDef> module_def) {
    sema::Init init{ctx, ast};
    init.visit(module_def);
    return module_def->module();
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

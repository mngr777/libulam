#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/env.hpp>

namespace ulam::sema {

Ref<Program> init(Context& ctx, Ref<ast::Root> ast);

Ref<Module>
init(Context& ctx, Ref<ast::Root> ast, Ref<ast::ModuleDef> module_def);

bool resolve(EvalEnv& env);

bool resolve(Context& ctx, Ref<Program> program);

} // namespace ulam::sema

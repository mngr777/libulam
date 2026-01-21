#pragma once
#include <libulam/ast.hpp>
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/env.hpp>

namespace ulam::sema {

Ref<Program> init(Context& ctx, Ref<ast::Root> ast);

bool resolve(Context& ctx, Ref<Program> program);

} // namespace ulam::sema

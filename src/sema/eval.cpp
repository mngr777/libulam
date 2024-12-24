#include <cassert>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/semantic/program.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::Eval] "
#include "src/debug.hpp"

namespace ulam::sema {

Eval::Eval(Context& ctx, Ref<ast::Root> ast):
    _ctx{ctx}, _ast{ast}, _ev{_ast->program()} {
    assert(_ast->program());
}

void Eval::eval(std::string& text) {
    Parser parser{_ctx, _ast->ctx().str_pool()};
    auto block = parser.parse_stmts(text);
    _ev.eval(ref(block));
}

} // namespace ulam::sema

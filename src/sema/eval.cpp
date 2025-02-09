#include <cassert>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/param_eval.hpp>
#include <libulam/semantic/program.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Eval] "
#    include "src/debug.hpp"
#endif

namespace ulam::sema {

Eval::Eval(Context& ctx, Ref<ast::Root> ast):
    _ctx{ctx}, _ast{ast}, _eval{_ast->program()} {
    assert(_ast->program());
}

void Eval::eval(const std::string& text) {
    Parser parser{_ctx, _ast->ctx().str_pool()};
    auto block = parser.parse_stmts(text);
    _eval.eval(ref(block));
}

} // namespace ulam::sema

#include <cassert>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/semantic/program.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Eval] "
#    include "src/debug.hpp"
#endif

namespace ulam::sema {

Eval::Eval(Context& ctx, Ref<ast::Root> ast):
    _ctx{ctx}, _ast{ast} {
    assert(_ast->program());
}

void Eval::eval(const std::string& text) {
    Parser parser{_ctx, _ast->ctx().str_pool(), _ast->ctx().text_pool()};
    auto block = parser.parse_stmts(text);
    visitor()->eval(ref(block));
}

Ptr<EvalVisitor> Eval::visitor() {
    return make<EvalVisitor>(_ast->program());
}

} // namespace ulam::sema

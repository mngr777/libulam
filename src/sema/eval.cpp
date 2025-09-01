#include <cassert>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/semantic/program.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Eval] "
#    include "src/debug.hpp"
#endif

namespace ulam::sema {

Eval::Eval(Context& ctx, Ref<ast::Root> ast): _ctx{ctx}, _ast{ast} {
    assert(_ast->program());
}

ExprRes Eval::eval(const std::string& text) {
    auto block = parse(text);
    return do_eval(ref(block));
}

Ptr<ast::Block> Eval::parse(const std::string& text) {
    Parser parser{_ctx, _ast->ctx().str_pool(), _ast->ctx().text_pool()};
    return parser.parse_stmts(text);
}

ExprRes Eval::do_eval(Ref<ast::Block> block) {
    EvalEnv env{_ast->program()};
    return env.eval(block);
}

} // namespace ulam::sema

#include "tests/ULAM/compiler.hpp"
#include <libulam/sema.hpp>

void Compiler::parse_string(const std::string& text, const std::string& name) {
    _parser.parse_string(text, name);
}

ulam::Ref<ulam::Program> Compiler::analyze() {
    auto ast = _parser.ast();
    ulam::Sema sema{_ctx.diag()};
    sema.analyze(ast);
    return ast->program();
}

void Compiler::compile(std::ostream& out) {
    // TODO
}

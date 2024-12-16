#include "tests/ULAM/compiler.hpp"
#include <libulam/sema.hpp>

void Compiler::parse_string(const std::string& text, const std::string& name) {
    _parser.parse_string(text, name);
}

void Compiler::compile(std::ostream& out) {
    auto ast = _parser.move_ast();

    ulam::Sema sema{_ctx.diag()};
    sema.analyze(ulam::ref(ast));

    // TODO
}

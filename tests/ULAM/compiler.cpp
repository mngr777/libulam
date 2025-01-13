#include "tests/ULAM/compiler.hpp"
#include <libulam/sema.hpp>
#include <libulam/sema/eval.hpp>
#include <utility>

void Compiler::parse_module_str(
    const std::string& text, const std::string& name) {
    auto module = _parser.parse_module_str(text, name);
    if (module)
        _ast->add(std::move(module));
}

ulam::Ref<ulam::Program> Compiler::analyze() {
    ulam::Sema sema{_ctx.diag()};
    sema.analyze(ulam::ref(_ast));
    return _ast->program();
}

void Compiler::compile(std::ostream& out) {
    ulam::sema::Eval eval{_ctx, ulam::ref(_ast)};
    eval.eval("test.test()");
}

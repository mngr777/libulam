#include "tests/sema/common.hpp"
#include "libulam/ast.hpp"
#include "libulam/ast/nodes/root.hpp"
#include "libulam/context.hpp"
#include "libulam/parser.hpp"
#include "libulam/sema.hpp"
#include "libulam/sema/eval.hpp"
#include "tests/ast/print.hpp"
#include <iostream>

ulam::Ptr<ulam::ast::Root>
analyze(const std::string& text, const std::string& module_name) {
    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();

    ulam::Parser parser{ctx, ast->ctx().str_pool(), ast->ctx().text_pool()};
    auto module = parser.parse_module_str(text, module_name);
    if (module)
        ast->add(std::move(module));

    ulam::Sema sema{ctx.diag(), ctx.sm()};
    sema.analyze(ulam::ref(ast));

    return ast;
}

ulam::Ptr<ulam::ast::Root>
analyze_and_print(const std::string& text, const std::string& module_name) {
    auto ast = analyze(text, module_name);
    test::ast::Printer printer{std::cout, ulam::ref(ast)};
    printer.print();
    return ast;
}

void analyze_print_and_run(
    const std::string& text,
    const std::string& module_name,
    const std::string& eval_text) {

    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();

    ulam::Parser parser{ctx, ast->ctx().str_pool(), ast->ctx().text_pool()};
    auto module = parser.parse_module_str(text, module_name);
    if (module)
        ast->add(std::move(module));

    ulam::Sema sema{ctx.diag(), ctx.sm()};
    sema.analyze(ulam::ref(ast));

    test::ast::Printer printer{std::cout, ulam::ref(ast)};
    printer.print();

    ulam::sema::Eval eval{ctx, ulam::ref(ast)};
    eval.eval(eval_text);
}

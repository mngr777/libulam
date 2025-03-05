#include "tests/sema/common.hpp"
#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/ast.hpp>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema.hpp>

ulam::Ptr<ulam::ast::Root>
analyze(const std::string& text, const std::string& module_name) {
    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();

    ulam::Parser parser{ctx, ast->ctx().str_pool()};
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

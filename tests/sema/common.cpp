#include "tests/sema/common.hpp"
#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema.hpp>

ulam::Ptr<ulam::ast::Root> analyze_and_print(std::string text) {
    ulam::Context ctx;
    ulam::Parser parser{ctx};

    parser.parse_string(text);
    auto ast = parser.move_ast();

    ulam::Sema sema{ctx.diag()};
    sema.analyze(ulam::ref(ast));

    test::ast::Printer printer{std::cout, ulam::ref(ast)};
    printer.print();

    return ast;
}

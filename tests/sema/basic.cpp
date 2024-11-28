#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema.hpp>

static const char* Program = R"END(
element A {
  typedef B.C D;

  Int(10) foo() {
    Int(5) a = 1;
    return a;
  }
}

quark B {}
)END";

int main() {
    ulam::Context ctx;
    ulam::Parser parser{ctx};

    std::string text{Program};
    parser.parse_string(text);
    auto ast = parser.move_ast();

    std::cout << text << "\n";
    test::ast::Printer p{std::cout};
    p.print(ulam::ast::ref(ast));

    ulam::Sema sema{ctx.diag()};
    sema.analyze(ulam::ast::ref(ast));
}

#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema.hpp>

// typedef Bits(8) Byte;
static const char* Program = R"END(
ulam 1;

quark ColorUtils {
  typedef Unsigned(8) Channel;
  typedef Channel ARGB[4];
}
)END";

int main() {
    ulam::Context ctx;
    ulam::Parser parser{ctx};

    std::string text{Program};
    parser.parse_string(text);
    auto ast = parser.move_ast();

    // std::cout << text << "\n";
    test::ast::Printer p{std::cout, ulam::ast::ref(ast)};
    p.print();

    ulam::Sema sema{ctx.diag()};
    sema.analyze(ulam::ast::ref(ast));
}

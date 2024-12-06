#include "tests/ast/print1.hpp"
#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema.hpp>

static const char* Program = R"END(
typedef Int(8) E;

element A {
  typedef B.C D;
  typedef E F;

  Int(8) foo() {
    Int(5) a = 1;
    return a;
  }
}

quark B {
  typedef Int(8) C;
  typedef A.F G;
}
)END";

int main() {
    ulam::Context ctx;
    ulam::Parser parser{ctx};

    std::string text{Program};
    parser.parse_string(text);
    auto ast = parser.move_ast();

    std::cout << text << "\n";
    test::ast::Printer p{std::cout, ulam::ast::ref(ast)};
    p.print();

    ulam::Sema sema{ctx.diag()};
    sema.analyze(ulam::ast::ref(ast));
}

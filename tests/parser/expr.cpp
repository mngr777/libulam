#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>

static const char* Program = R"END(
quark Q(B.C param1 = 0xff) {
  Int a = 1;

  Int& bar(constant Int &b[1], Unsigned c = 1) {
    return a;
  }
}

element A {
  typedef B.C D;

  B.C.D a = 1, b, c = 2;
  constant C.D.E d = 3;

  C(2).D.E foo(F.G c = 1) {
    H(2) a = 1;
    if (a > 0)
      ++a;
    else
      --c;
    for (; a < 5; a++) {}
  }
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
}

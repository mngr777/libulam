#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/ast.hpp>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>

static const char* Program = R"END(
quark Q(B.C param1 = 0xff) {
  Int a = 1;

  Int& bar(constant Int &b[1], Unsigned c = 1) {
    return a;
  }

  Int operator[](Int i) { return i; }

  Q operator+=(Q other) {
    Q res;
    res.a = a + other.a;
    return res;
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
    auto ast = ulam::make<ulam::ast::Root>();
    ulam::Parser parser{ctx, ast->ctx().str_pool(), ast->ctx().text_pool()};

    std::string text{Program};
    ast->add(parser.parse_module_str(text, "A"));

    std::cout << text << "\n";
    test::ast::Printer p{std::cout, ulam::ref(ast)};
    p.print();
}

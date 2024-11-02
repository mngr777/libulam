#include "libulam/parser.hpp"
#include "libulam/src_mngr.hpp"
#include "tests/ast/print.hpp"
#include <iostream>

/*
    typedef C.D B;

  B mB = 123;
 */

static const char* Program = R"END(
element A {
  typedef B.C D;

  B.C.D a = 1, b, c = 2;
  C.D.E d = 3;

  C(2).D.E foo(F.G c = 1) {
    H(2) a = 1;
    for (; a < 5; ++a + 1) {}
  }
}
)END";

int main() {
    ulam::SrcMngr sm;
    ulam::Parser parser{sm};

    std::string text{Program};
    auto ast = parser.parse_str(text);

    std::cout << text << "\n";
    test::ast::Printer p{std::cout};
    p.print(ulam::ast::ref(ast));
}

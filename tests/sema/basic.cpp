#include "tests/sema/common.hpp"

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

int main() { analyze_and_print(Program, "A"); }

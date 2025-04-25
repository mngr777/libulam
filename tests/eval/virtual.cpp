#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A : B {

}

quark B : C {
  virtual Int foo() {
    return 2;
  }
}

quark C {
  virtual Int foo() {
    return 1;
  }
}

)END";

int main() {
    analyze_print_and_run(Program, "A", R"END(
A a;
if (a as C) {
  a.foo();
}
)END");
}

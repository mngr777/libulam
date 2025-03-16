#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A : B {
  @Override Int foo() {
    return 1;
  }
}

quark B {
  virtual Int foo();

  Int bar() {
    return foo();
  }
}

)END";

int main() {
    analyze_print_and_run(Program, "A", "A a; a.bar();");
}

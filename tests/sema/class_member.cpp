#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A {
  B b;
}

quark B {
  A a;
}

)END";

int main() {
    analyze_and_print(Program);
}

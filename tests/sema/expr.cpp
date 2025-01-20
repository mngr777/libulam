#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A {
  constant Int mC1 = 1 + 1u;
}
)END";

int main() { analyze_and_print(Program, "A"); }

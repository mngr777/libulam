#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A {
  constant Int mC1 = (Unsigned)(1 + 1);
}
)END";

int main() { analyze_and_print(Program, "A"); }

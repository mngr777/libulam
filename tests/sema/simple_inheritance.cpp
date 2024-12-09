#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark A {}

quark B {}

quark C : A {}

quark D : A + B {}
)END";

int main() {
    analyze_and_print(Program);
}

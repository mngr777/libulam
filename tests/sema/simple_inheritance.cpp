#include "tests/sema/common.hpp"

static const char* Program = R"END(
quark B(Int cP) {}
quark A : B(1) {}
)END";

int main() {
    analyze_and_print(Program);
}

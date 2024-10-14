#include "libulam/token.hpp"
#include "src/context.hpp"
#include "src/parser.hpp"
#include "src/lexer.hpp"
#include "src/source.hpp"
#include "src/source_manager.hpp"
#include "tests/ast/print.hpp"
#include <iostream>
#include <memory_resource>

/*
    typedef C.D B;

  B mB = 123;
 */

static const char* Program = R"END(
element A {
  C.D foo(E.F c = 1) {
  }
}
)END";

int main() {
    auto res = std::pmr::get_default_resource();
    std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ulam::SourceManager sm{res};
    std::string text{Program};
    auto src = sm.add_string("", text);

    ulam::Context ctx{res};
    ulam::Parser parser{ctx, src->stream()};
    auto ast = parser.parse();

    std::cout << text << "\n";
    test::ast::Printer p{std::cout};
    p.print(ast.get());
}

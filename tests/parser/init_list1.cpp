#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/ast.hpp>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>


static const char* Program = R"END(
transient Ops {
  constant Unsigned cNUM = 1;
  constant String mNames[cNUM] = {
    ">"
  };
}

)END";

int main() {
    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();
    ulam::Parser parser{ctx, ast->ctx()};

    std::string text{Program};
    ast->add(parser.parse_module_str(text, "Ops"));

    std::cout << text << "\n";
    test::ast::Printer p{std::cout, ulam::ref(ast)};
    p.print();
}

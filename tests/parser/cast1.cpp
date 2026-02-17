#include "tests/ast/print.hpp"
#include <iostream>
#include <libulam/ast.hpp>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>


static const char* Program = R"END(
quark Foo {
  Void bar() {
    baz.call(x * (C2D.Coord) (Drawable.cUNITS_PER_SITE / 2));
  }
}
)END";

int main() {
    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();
    ulam::Parser parser{ctx, ast->ctx()};

    std::string text{Program};
    ast->add(parser.parse_module_str(text, "Foo"));

    std::cout << text << "\n";
    test::ast::Printer p{std::cout, ulam::ref(ast)};
    p.print();
}

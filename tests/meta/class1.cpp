#include "libulam/ast.hpp"
#include "libulam/context.hpp"
#include "libulam/parser.hpp"
#include "libulam/sema.hpp"
#include <iostream>

static const char* Program = R"END(ulam 1;
/**
  Element A description

  \symbol A
  \version 2
*/
element A {}
)END";

int main() {
    ulam::Context ctx;
    auto ast = ulam::make<ulam::ast::Root>();
    ulam::Parser parser{ctx, ast->ctx()};

    std::string text{Program};
    ast->add(parser.parse_module_str(text, "A.ulam"));

    auto program = ulam::sema::init(ctx, ulam::ref(ast));
    ulam::sema::resolve(ctx, program);

    auto module = program->module("A");
    assert(module);

    auto sym = module->get("A");
    sym->accept(
        [&](ulam::Ref<ulam::Class> cls) {
            const auto& meta = cls->node()->meta();
            assert(!meta.empty());
            std::cout << cls->name() << " meta:\n" << meta << "\n";
        },
        [&](auto&&) { assert(false); });
}

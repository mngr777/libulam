#include "libulam/token.hpp"
#include "src/context.hpp"
#include "src/lexer.hpp"
#include "src/source.hpp"
#include "src/source_manager.hpp"
#include <iostream>
#include <memory_resource>

static const char* Program = R"END(
ulam 1;

quark A {
  Int mCount = 0;

  Bool countToTen() {
    DebugUtils du;
    ++mCount;
    if (mCount < 10) {
      du.print(mCount)
    } else {
      du.print("ten!")
      mCount = 0;
    }
  }
}

element A : B {
  @Override Void behave() {
    if (countToTen)
      doStuff();
  }

  Void doStuff() {
    DebugUtils du;
    du.print("doing stuff...");
  }
}
)END";

int main() {
    auto res = std::pmr::get_default_resource();
    ulam::Context ctx{res};
    ulam::SourceManager sm{res};
    std::string text{Program};
    auto source = sm.add_string("program", text);
    ulam::Lexer lexer{ctx, source->stream()};
    ulam::Token token;
    do {
        lexer >> token;
        auto type_str = ulam::tok::type_str(token.type);
        std::cout << (!type_str.empty() ? type_str : ulam::tok::type_name(token.type));
        if (token.type == ulam::tok::Name || token.type == ulam::tok::Number ||
            token.type == ulam::tok::String) {
            std::cout << " `"
                      << ((token.type == ulam::tok::Name)
                              ? ctx.name_str(token.str_id)
                              : ctx.value_str(token.str_id))
                      << "'";
        }
        std::cout << "\n";
    } while (token.type != ulam::tok::None);
}

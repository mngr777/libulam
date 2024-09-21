#include "libulam/token.hpp"
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
    ulam::SourceManager sm(std::pmr::get_default_resource());
    std::string text{Program};
    auto source = sm.add_string("program", text);
    ulam::Lexer lexer{source->stream()};
    ulam::Token token;
    do {
        lexer >> token;
        std::cout << ulam::tok::type_to_str(token.type) << "\n";
    } while (token.type != ulam::tok::None);
}

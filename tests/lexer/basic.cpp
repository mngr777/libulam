#include "libulam/token.hpp"
#include "src/context.hpp"
#include "src/lexer.hpp"
#include "src/source.hpp"
#include "src/source_manager.hpp"
#include <array>
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
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    ulam::Context ctx{res};
    ulam::SourceManager sm{res};
    std::string text{Program};
    auto source = sm.add_string("program", text);
    ulam::Lexer lexer{ctx, source->stream()};
    ulam::Token token;
    do {
        lexer >> token;
        if (token.is(ulam::tok::Name)) {
            std::cout << "`" << ctx.name_str(token.str_id) << "'";
        } else if (token.is(ulam::tok::Number, ulam::tok::String)) {
            std::cout << ctx.value_str(token.str_id);
        } else {
            const char* str = token.type_str();
            std::cout << (str ? str : token.type_name());
        }
        std::cout << "\n";
    } while (token.type != ulam::tok::Eof);
}

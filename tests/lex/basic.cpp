#include <iostream>
#include <libulam/context.hpp>
#include <libulam/parser/lex.hpp>
#include <libulam/parser/token.hpp>
#include <libulam/src.hpp>
#include <libulam/src_man.hpp>

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
    ulam::Context ctx;
    std::string text{Program};
    auto src = ctx.src_man().string(text, "A");
    ulam::Lex lex{ctx.diag(), ctx.src_man(), src->id(), src->content()};
    ulam::Token token;
    do {
        lex.lex(token);
        if (token.in(ulam::tok::Ident, ulam::tok::Number, ulam::tok::String)) {
            std::cout << "`"
                      << ctx.src_man().str_at(token.loc_id(), token.size())
                      << "'";
        } else {
            const char* str = token.type_str();
            std::cout << (str ? str : token.type_name());
        }
        std::cout << "\n";
    } while (!token.is(ulam::tok::Eof));

    token.is(ulam::tok::Ident);
}

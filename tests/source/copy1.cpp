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
})END";

int main() {
    ulam::SourceManager sm(std::pmr::get_default_resource());
    std::string text {Program}, copy;
    auto source = sm.add_string("program", text);
    char ch;
    std::istream& out = source->stream();
    while (true) {
        out.get(ch);
        if (out.eof())
            break;
        copy += ch;
    };
    std::cout << "original:" << text << "\n--------------------\n";
    std::cout << "copy:" << copy << "\n--------------------\n";
    std::cout << "text length: " << text.size() << "\n";
    std::cout << "copy length: " << copy.size() << "\n";
    return (text == copy) ? 0 : -1;
}

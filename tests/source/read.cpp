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
    auto source = sm.add_string("program", Program);
    std::string out;
    while (!source->out().eof()) {
        source->out().get();
    }
}

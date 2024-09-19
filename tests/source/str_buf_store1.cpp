#include "libulam/types.hpp"
#include "src/source/line_buf.hpp"
#include "src/source/line_storage.hpp"
#include <iostream>
#include <memory_resource>
#include <sstream>

static const char* Program = R"END(
ulam 1;

element A {
  void foo() {
    EventWindow ew;
    if (ew.isEmpty(1)) {
      ew[1] = ew[0];
    }
  }
}
)END";

int main() {
    std::pmr::monotonic_buffer_resource res(std::pmr::get_default_resource());
    std::stringstream ss{Program};
    ulam::src::LineStorage lines{&res};
    ulam::src::LineBuf line_buf{lines, ss, &res};
    std::istream is(&line_buf);
    std::string line;
    char ch;
    ulam::LineNum line_num = 0;
    while (true) {
        // load new line
        is.get(ch);
        if (is.eof())
            break;

        ++line_num;
        line_buf.store();

        // scroll to end of line
        if (ch != '\n')
            std::getline(is, line);
    }

    // are all the lines stored?
    if (lines.size() != line_num) {
        std::cerr << "Lines in program: " << line_num << ", stored: " << lines.size();
        return -1;
    }

    std::string orig {Program};
    std::string copy;
    for (ulam::LineNum n = 1; n <= lines.size(); ++n) {
        if (!lines.has(n)) {
            std::cerr << "Line " << n << " is not stored\n";
            return -1;
        }
        copy += lines.get(n).line;
    }
    std::cerr << "Original (" << orig.size() << " chars):\n"
              << Program << "\n"
              << "--------------------\n";
    std::cerr << "Copy (" << copy.size() << " chars):\n"
              << copy << "\n"
              << "--------------------\n";
    return (copy == Program) ? 0 : -1;
}

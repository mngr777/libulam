#include "src/memory/notepad.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <memory_resource>
#include <string_view>

int main() {
    const std::size_t PageSize = 16;

    std::array<std::string, 4> l1{
        std::string{"page1: lorem"}, std::string{"page2: ipsum"},
        std::string{"page2: dolor"},
        std::string{"page3: notepad should be able to accomodate strings for "
                    "arbitrary length"}};

    std::pmr::monotonic_buffer_resource res(std::pmr::get_default_resource());
    ulam::mem::Notepad n1(&res, PageSize);
    for (const auto& s : l1) {
        auto written = n1.write(s);
        std::cout << "string: `" << s << "', written: `" << written << "'\n";
        assert(written.data() != s.data());
        if (s != written)
            return -1;
    }

    return 0;
}

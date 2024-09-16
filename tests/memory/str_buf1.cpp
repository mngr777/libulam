#include "src/memory/str_buf.hpp"
#include <iostream>
#include <memory_resource>
#include <string_view>

int main() {
    const std::size_t InitialSize = 16;
    const std::size_t GrowBy = 8;

    std::pmr::monotonic_buffer_resource res(std::pmr::get_default_resource());
    ulam::mem::StrBuf buf1(&res, InitialSize, GrowBy);
    std::string_view sv1("this is a test");

    std::cout << sv1 << "\n";
    std::cout << "address: " << static_cast<void*>(buf1.data()) << "\n";
    sv1.copy(buf1.data(), sv1.size());
    buf1.grow();

    std::cout << "address: " << static_cast<void*>(buf1.data()) << "\n";
    std::string_view sv2(buf1.data(), sv1.size());
    std::cout << sv2 << "\n";

    return sv2 == sv1 ? 0 : -1;
}

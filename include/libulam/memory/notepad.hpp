#pragma once
#include <cassert>
#include <cstddef>
#include <string_view>

namespace ulam::mem {

class Notepad {
private:
    struct Page {
        Page(char* data, std::size_t size): data(data), cur(data), size(size) {}

        std::size_t freespace() { return size - (cur - data); }

        char* data;
        char* cur;
        std::size_t size;
        Page* next{nullptr};
    };

public:
    explicit Notepad(std::size_t pagesize = 512): _pagesize(pagesize) {
        assert(pagesize % 8 == 0);
    }
    ~Notepad();

    Notepad(const Notepad&) = delete;
    Notepad& operator=(const Notepad&) = delete;

    const std::string_view write(const std::string_view text);

private:
    void alloc_next(const std::size_t size);

    const std::size_t _pagesize;
    Page* _first{nullptr};
    Page* _last{nullptr};
};

} // namespace ulam::mem

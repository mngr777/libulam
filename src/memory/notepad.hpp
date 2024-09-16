#pragma once
#include <memory_resource>
#include <string_view>

namespace ulam::mem {

class Notepad {
private:
    struct Page {
        Page(char* data, std::size_t size):
            data(data), cur(data), size(size) {}

        std::size_t freespace() { return size - (cur - data); }

        char* data;
        char* cur;
        std::size_t size;
        Page* next {nullptr};
    };

public:
    static const std::size_t HeadSize = sizeof(Page);

    Notepad(std::pmr::memory_resource* res, std::size_t pagesize = 8192):
        _res(res), _pagesize(pagesize) {} // TODO: alignment and corresponting size
    ~Notepad();

    Notepad(const Notepad&) = delete;
    Notepad& operator=(const Notepad&) = delete;

    std::string_view write(std::string_view text);

private:
    void alloc_next(std::size_t size);

    std::pmr::memory_resource* _res;
    const std::size_t _pagesize;
    Page* _first {nullptr};
    Page* _last {nullptr};
};

} // namespace ulam::mem


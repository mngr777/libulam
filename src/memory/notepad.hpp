#pragma once
#include <memory_resource>
#include <string_view>

namespace ulam::mem {

class Notepad {
private:
    using CharT = std::string_view::value_type;

    struct Page {
        Page(CharT* data, std::size_t size):
            data(data), cur(data), size(size) {}

        std::size_t freespace() { return size - (cur - data); }

        CharT* data;
        CharT* cur;
        std::size_t size;
        Page* next {nullptr};
    };

public:
    static const std::size_t HeadSize = sizeof(Page);

    explicit Notepad(std::pmr::memory_resource* res, std::size_t pagesize = 8192):
        _pool(res), _pagesize(pagesize) {} // TODO: alignment and corresponting size

    Notepad(const Notepad&) = delete;
    Notepad& operator=(const Notepad&) = delete;

    const std::string_view write(const std::string_view text);

private:
    void alloc_next(const std::size_t size);

    std::pmr::unsynchronized_pool_resource _pool;
    const std::size_t _pagesize;
    Page* _first {nullptr};
    Page* _last {nullptr};
};

} // namespace ulam::mem


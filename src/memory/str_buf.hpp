#pragma once
#include <cstddef>
#include <memory_resource>
#include <string>

namespace ulam::mem {

// TODO: options as struct

class StrBuf {
public:
    using CharT = std::string::value_type;

    StrBuf(
        std::pmr::memory_resource* res,
        std::size_t initial_size = 512,
        std::size_t grow_by = 256);
    ~StrBuf();

    const std::size_t grow();
    void resize(const std::size_t size);

    CharT* data() { return _data; }
    std::size_t size() { return _size; }

    CharT* begin() { return _data; }
    CharT* end() { return _data + _size; }

private:
    std::pmr::polymorphic_allocator<CharT> _pa;
    std::size_t _size;
    std::size_t _grow_by;
    CharT* _data;
};

} // namespace ulam::mem

#pragma once
#include <cassert>
#include <cstddef>
#include <memory>

namespace ulam::mem {

class BufRef {
public:
    BufRef(const char* data, std::size_t size): _data{data}, _size{size} {}

    const char* start() const { return _data; }
    const char* end() const { return _data + _size; }

    operator bool() const { return _size > 0; }

private:
    const char* _data;
    std::size_t _size;
};

class Buf {
public:
    // TODO: alignment
    Buf(): _data{nullptr}, _size{0} {}

    Buf(std::size_t size): _data{new char[size]()} {}

    BufRef ref() const { return {_data.get(), _size}; }

    char* start() { return _data.get(); }
    const char* start() const { return _data.get(); }

    char* end() { return _data.get() + _size; }
    const char* end() const { return _data.get() + _size; }

    std::size_t size() const { return _size; }

    operator bool() const { return _size > 0; }

private:
    std::unique_ptr<char[]> _data;
    std::size_t _size;
};

} // namespace ulam::mem

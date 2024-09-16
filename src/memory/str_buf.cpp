#include "src/memory/str_buf.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory_resource>

#ifdef DEBUG_STR_BUF
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::mem::StrBuf] "
static std::size_t BytesAllocated = 0;
#endif
#include "src/debug.hpp"

namespace ulam::mem {

StrBuf::StrBuf(
    std::pmr::memory_resource* res,
    std::size_t initial_size,
    std::size_t grow_by):
    _pa(res ? res : std::pmr::get_default_resource()),
    _size(initial_size),
    _grow_by(grow_by),
    _data(_pa.allocate(_size)) {
#ifdef ULAM_DEBUG
    BytesAllocated += _size;
    debug() << _size << " bytes allocated, " << BytesAllocated << " total\n";
#endif
}

StrBuf::~StrBuf() {
    if (_data) {
        assert(_size > 0);
        _pa.deallocate(_data, _size);
#ifdef ULAM_DEBUG
        assert(BytesAllocated >= _size);
        BytesAllocated -= _size;
        debug() << _size << " bytes deallocated, " << BytesAllocated << " total\n";
#endif
    }
}

const std::size_t StrBuf::grow() {
    resize(_size + _grow_by);
    return _size;
}

void StrBuf::resize(const std::size_t size) {
    assert(size > 0 && "String buffer size cannot be zero");
    assert(_data && "String buffer not initialized");
    auto newdata = _pa.allocate(size);
    std::memcpy(newdata, _data, std::min(_size, size));
    _pa.deallocate(_data, _size);
#ifdef ULAM_DEBUG
    BytesAllocated = BytesAllocated + size - _size;
    debug() << size << " bytes allocated, " << _size << " deallocated, "
            << BytesAllocated << " total\n";
#endif
    _data = newdata;
    _size = size;
}

} // namespace ulam::mem

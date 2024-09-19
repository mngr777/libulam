#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include "src/memory/notepad.hpp"

namespace ulam::mem {

const std::string_view Notepad::write(const std::string_view text) {
    const auto len = text.size();
    if (!_last || _last->freespace() < len + 1)
        alloc_next(std::max(_pagesize, len + 1));
    assert(_last);
    CharT* cur = _last->cur;
    text.copy(cur, len);
    _last->cur += len + 1;
    return {cur, len};
}

void Notepad::alloc_next(const std::size_t size) {
    Page* next = static_cast<Page*>(_pool.allocate(sizeof(Page) + size * sizeof(CharT)));
    if (!_first)
        _first = next;
    if (_last)
        _last->next = next;
    _last = next;
    *next = {reinterpret_cast<CharT*>(&next[1]), size};
}

} // namespace ulam::mem

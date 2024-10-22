#include <algorithm>
#include "src/memory/notepad.hpp"

namespace ulam::mem {

Notepad::~Notepad() {
    auto cur = _first;
    while (cur) {
        auto next = cur->next;
        delete[] reinterpret_cast<char*>(cur);
        cur = next;
    }
}

const std::string_view Notepad::write(const std::string_view text) {
    const auto len = text.size();
    if (!_last || _last->freespace() < len + 1)
        alloc_next(std::max(_pagesize, len + 1));
    assert(_last);
    char* cur = _last->cur;
    text.copy(cur, len);
    cur[len] = '\0';
    _last->cur += len + 1;
    return {cur, len};
}

void Notepad::alloc_next(const std::size_t size) {
    const std::size_t Size = (sizeof(Page) + size * sizeof(char) + 8) & ~7;
    Page* next = reinterpret_cast<Page*>(new char[Size]);
    if (!_first)
        _first = next;
    if (_last)
        _last->next = next;
    _last = next;
    *next = {reinterpret_cast<char*>(&next[1]), size};
}

} // namespace ulam::mem

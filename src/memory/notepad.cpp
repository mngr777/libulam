#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include "src/memory/notepad.hpp"

#ifdef DEBUG_NOTEPAD
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::mem::Notepad] "
static std::size_t BytesAllocated = 0;
#endif
#include "src/debug.hpp"

namespace ulam::mem {

Notepad::~Notepad() {
    Page* page = _first;
    while (page) {
        Page* next = page->next;
#ifdef ULAM_DEBUG
        assert(BytesAllocated >= page->size);
        BytesAllocated -= page->size;
        debug() << page->size << "bytes deallocated, " << BytesAllocated << " total\n";
#endif
        _res->deallocate(page, sizeof(Page) + page->size * sizeof(char));
        page = next;
    }
}

std::string_view Notepad::write(std::string_view text) {
    const auto len = text.size();
    if (!_last || _last->freespace() < len + 1)
        alloc_next(std::max(_pagesize, len + 1));
    assert(_last);
    char* cur = _last->cur;
    text.copy(cur, len);
    _last->cur += len + 1;
    return {cur, len};
}

void Notepad::alloc_next(std::size_t size) {
    Page* next = static_cast<Page*>(_res->allocate(sizeof(Page) + size * sizeof(char)));
#ifdef ULAM_DEBUG
    BytesAllocated += size;
    debug() << size << " bytes allocated, " << BytesAllocated << " total\n";
#endif
    if (!_first)
        _first = next;
    if (_last)
        _last->next = next;
    _last = next;
    *next = {reinterpret_cast<char*>(&next[1]), size};
}

} // namespace ulam::mem
